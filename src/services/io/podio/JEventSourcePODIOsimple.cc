// Copyright 2022, David Lawrence
// Subject to the terms in the LICENSE file found in the top-level directory.
//
//
// This is a JANA event source that uses PODIO to read from a ROOT
// file created using the EDM4hep Data Model.
//
// This uses the podio supplied RootReader and EventStore classes. Thus,
// it is limited to processing only a single event at a time.

#include "JEventSourcePODIOsimple.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <filesystem>

#include <JANA/JFactoryGenerator.h>

// podio specific includes
#include <podio/EventStore.h>
#include <podio/IReader.h>
#include <podio/UserDataCollection.h>
#include <podio/podioVersion.h>

#include <fmt/format.h>

// This file is generated automatically by make_datamodel_glue.py
#include "datamodel_glue.h"


//------------------------------------------------------------------------------
// CopyPodioToJEventSimpleT
//
/// This is called from the GetEvent method below by way of the generated code
/// in datamodel_glue.h. This will make copies of the high-level podio objects
/// that wrap the same "Obj" mid-level objects managed by the collection.
/// The copies of the high-level objects are managed by JANA and can be accessed
/// via the standard JANA event.Get<>() mechanism.
///
/// \tparam T           podio high-level data type (e.g. edm4hep::EventHeader)
/// \tparam Tcollection podio collection type (e.g. edm4hep::EventHeaderCollection)
/// \param collection   pointer to the podio collection (e.g. edm4hep::EventHeaderCollection*)
/// \param name         name of the collection which will be used as the factory tag for these objects
/// \param event        JANA JEvent to copy the data objects into
//------------------------------------------------------------------------------
template <typename T, typename Tcollection>
void CopyToJEventSimpleT(const Tcollection *collection, const std::string &name, std::shared_ptr<JEvent> &event){

    std::vector<const T*> tptrs;
    for( int i=0; i<collection->size(); i++){
        const auto &obj = (*collection)[i];  // Create new object of type "T" on stack that uses existing "Obj" object.
        tptrs.push_back( new T(obj) ); // Create new object of type "T" on heap that uses existing "Obj" object.
    }
    event->Insert( tptrs, name );
}

//------------------------------------------------------------------------------
// Constructor
//
///
/// \param resource_name  Name of root file to open (n.b. file is not opened until Open() is called)
/// \param app            JApplication
//------------------------------------------------------------------------------
JEventSourcePODIOsimple::JEventSourcePODIOsimple(std::string resource_name, JApplication* app) : JEventSource(resource_name, app) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name

    // Tell JANA that we want it to call the FinishEvent() method.
    EnableFinishEvent();
}

//------------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------------
JEventSourcePODIOsimple::~JEventSourcePODIOsimple() {
    LOG << "Closing Event Source for " << GetResourceName() << LOG_END;
}

//------------------------------------------------------------------------------
// Open
//
/// Open the root file and read in metadata.
//------------------------------------------------------------------------------
void JEventSourcePODIOsimple::Open() {

    // Allow user to specify to recycle events forever
    GetApplication()->SetDefaultParameter("PODIO:RUN_FOREVER", m_run_forever, "set to true to recycle through events continuously");

    bool print_type_table = false;
    GetApplication()->SetDefaultParameter("PODIO:PRINT_TYPE_TABLE", print_type_table, "Print list of collection names and their types");

    try {
        // Have PODIO reader open file and get the number of events from it.
        reader.openFile( GetResourceName() );
        if( ! reader.isValid() ) throw std::runtime_error( fmt::format("podio ROOTReader says {} is invalid", GetResourceName()) );

        auto version = reader.currentFileVersion();
        bool version_mismatch = version.major > podio::version::build_version.major;
        version_mismatch |= (version.major == podio::version::build_version.major) && (version.minor>podio::version::build_version.minor);
        if( version_mismatch ) {
            std::stringstream ss;
            ss << "Mismatch in PODIO versions! " << version << " > " << podio::version::build_version;
            // FIXME: The podio ROOTReader is somehow failing to read in the correct version numbers from the file
//            throw JException(ss.str());
        }

        LOG << "PODIO version: file=" << version << " (executable=" << podio::version::build_version << ")" << LOG_END;

        Nevents_in_file = reader.getEntries();
        LOG << "Opened PODIO file \"" << GetResourceName() << "\" with " << Nevents_in_file << " events" << LOG_END;

        store.setReader(&reader);
        reader.readEvent();

        if( print_type_table ) PrintCollectionTypeTable();

    }catch (std::exception &e ){
        LOG_ERROR(default_cerr_logger) << e.what() << LOG_END;
        throw JException( fmt::format( "Problem opening file \"{}\"", GetResourceName() ) );
    }
}

//------------------------------------------------------------------------------
// GetEvent
//
/// Read next event from file and copy its objects into the given JEvent.
///
/// \param event
//------------------------------------------------------------------------------
void JEventSourcePODIOsimple::GetEvent(std::shared_ptr<JEvent> event) {

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.

    // Check if we have exhausted events from file
    if( Nevents_read >= Nevents_in_file ) {
        if( m_run_forever ){
            Nevents_read = 0;
        }else{
            reader.closeFile();
            throw RETURN_STATUS::kNO_MORE_EVENTS;
        }
    }

    // The podio supplied RootReader and EventStore are not multi-thread capable so limit to a single event in flight
    if( m_inflight ) throw RETURN_STATUS ::kBUSY;
    m_inflight = true;

    // Read the specified event into the EventStore and make the EventStore pointer available via JANA
    store.clear();
    reader.endOfEvent();
    reader.goToEvent( Nevents_read++ );
    auto fac = event->Insert( &store );
    fac->SetFactoryFlag(JFactory::NOT_OBJECT_OWNER); // jana should not delete this

    // Loop over collections in EventStore and copy pointers to their contents into jevent
    auto collectionIDtable = store.getCollectionIDTable();
    for( auto id : collectionIDtable->ids() ){
         podio::CollectionBase *coll={nullptr};
        if( store.get(id, coll) ){
            auto name = collectionIDtable->name(id);
            auto className = coll->getTypeName();
            CopyToJEventSimple( className, name, coll, event);

            if( name == "EventHeader"){
                auto ehc = reinterpret_cast<const edm4hep::EventHeaderCollection *>(coll);
                if( ehc && ehc->size() ){
                    event->SetEventNumber( (*ehc)[0].getEventNumber());
                    event->SetRunNumber( (*ehc)[0].getRunNumber());
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// FinishEvent
//
/// Clear the flag used to limit us to a single event in flight.
///
/// \param event
//------------------------------------------------------------------------------
void JEventSourcePODIOsimple::FinishEvent(JEvent &event){

    m_inflight = false;
}

//------------------------------------------------------------------------------
// GetDescription
//------------------------------------------------------------------------------
std::string JEventSourcePODIOsimple::GetDescription() {

    /// GetDescription() helps JANA explain to the user what is going on
    return "PODIO root file (simple)";
}

//------------------------------------------------------------------------------
// CheckOpenable
//
/// Return a value from 0-1 indicating probability that this source will be
/// able to read this root file. Currently, it simply checks that the file
/// name contains the string ".root" and if does, returns a small number (0.02).
/// This will need to be made more sophisticated if the alternative root file
/// formats need to be supported by other event sources.
///
/// \param resource_name name of root file to evaluate.
/// \return              value from 0-1 indicating confidence that this source can open the given file
//------------------------------------------------------------------------------
template <>
double JEventSourceGeneratorT<JEventSourcePODIOsimple>::CheckOpenable(std::string resource_name) {

    // If source is a root file, given minimal probability of success so we're chosen
    // only if no other ROOT file sources exist.
    return (resource_name.find(".root") != std::string::npos ) ? 0.02 : 0.0;
}

//------------------------------------------------------------------------------
// PrintCollectionTypeTable
//
/// Print the list of collection names from the currently open file along
/// with their types. This will be called automatically when the file is
/// open if the PODIO:PRINT_TYPE_TABLE variable is set to a non-zero value
//------------------------------------------------------------------------------
void JEventSourcePODIOsimple::PrintCollectionTypeTable(void) {

    // First, get maximum length of the collection name strings so
    // we can print nicely aligned columns.
    size_t max_name_len = 0;
    size_t max_type_len = 0;
    std::map<std::string, std::string> collectionNames;
    auto collectionIDtable = store.getCollectionIDTable();
    for (auto id : collectionIDtable->ids()) {
        auto name = collectionIDtable->name(id);
        podio::CollectionBase *coll = {nullptr};
        if (store.get(id, coll)) {
            auto type = coll->getTypeName();
            max_name_len = std::max(max_name_len, name.length());
            max_type_len = std::max(max_type_len, type.length());
            collectionNames[name] = type;
        }
    }

    // Print table
    std::cout << std::endl;
    std::cout << "Available Collections" << std::endl;
    std::cout << std::endl;
    std::cout << "Collection Name" << std::string(max_name_len + 2 - std::string("Collection Name").length(), ' ')
              << "Data Type" << std::endl;
    std::cout << std::string(max_name_len, '-') << "  " << std::string(max_name_len, '-') << std::endl;
    for (auto &[name, type] : collectionNames) {
        std::cout << name + std::string(max_name_len + 2 - name.length(), ' ');
        std::cout << type << std::endl;
    }
    std::cout << std::endl;

}
