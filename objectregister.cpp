#include "objectregister.hpp"
#include "timer.hpp"
#include "logging.hpp"
#include "inifile.hpp"


// define boost logging stuff.
BOOST_DEFINE_LOG(objectregister, "objectregister")

/**
\file
Implementation of ObjectRegister and helper classes/methods.
*/

#ifdef _WIN32
// ignore warning because passing pointer to this to base member in constructor.
#pragma warning( disable : 4355 )
#endif

// separate componenets of a register string with a dot
const char* gRegStringSeps = ".";
const char* gRegStringFunction = "function";
const char* gRegStringCollection = "collection";
const char* gRegStringFile = "file";

bool ObjectRegister::IsValidVariableName(string& arName) {
    // a valid variable is of the form:
    // (alpha) (alphanum | '_')*

    if (arName.length() == 0 || !isalpha(arName[0])) {
        return false;
    }

    for (size_t i = 1; i < arName.length(); ++i) {
        if (!isalnum(arName[i]) && arName[i] != '_') {
            return false;
        }
    }

    return true;
}

// construct a string of 1 to 3 components, separated by 
// gRegStringSeps
std::string RegisterString( const std::string& aFirst,
                            const std::string& aSecond,
                            const std::string& aThird) {

    std::stringstream sstream;
    sstream << aFirst;
    if (aSecond != "") {
        sstream << gRegStringSeps << aSecond;
        if (aThird != "") {
            sstream << gRegStringSeps << aThird;
        }
    }
    return sstream.str();
};

std::string FunctionRegisterString(const std::string& aFunctionName) {
    return RegisterString(  gRegStringFunction,
                            aFunctionName,
                            "");
}

std::string FileRegisterString(const std::string& aFileName) {
    return RegisterString(  gRegStringFile,
                            aFileName,
                            "");
}

std::string CollectionRegisterString(const std::string& aCollectionName) {
    return RegisterString(  gRegStringCollection,
                            aCollectionName,
                            "");
}

// This function takes object data in "inifile" form and makes the objects,
// using an ObjectFactory object.
void MakeObjectsFromIniFile(ObjectFactory& arFactory, 
                            istream* apStream, 
                            string aStreamName,
                            FileSystem& arFileSystem,
                            ObjectRegister& arRegister) {

    EnhancedIniFile inifile(apStream, &arFileSystem, &arRegister, aStreamName);
    for (GroupMap::iterator iter = inifile.Begin();
        iter != inifile.End();
        ++iter) {
            
        string groupname = (*iter).first;
        IniFileGroup group = (*iter).second;
        string classname = inifile.FindClassNameForGroup(groupname);
        if (classname != "") {
            string instancename = 
                inifile.FindClassInstanceNameForGroup(groupname);
            map<string, string>* nvp = 
                inifile.FindClassInstance(classname, instancename);
                    
            try {
                arFactory.Make(classname, instancename, *nvp);    
            } catch(TemsimException& e) {
                throw TemsimException("Failed creating object '" 
                    + instancename + "' defined in " + group.FileName() + " (" 
                    + e.what() + ")");
            }
        }
    }
}




