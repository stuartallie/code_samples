#ifndef _OBJECTREGISTER_HPP_
#define _OBJECTREGISTER_HPP_

#include <iostream>
#include <string>
#include <map>
#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/function.hpp>
#include <boost/format.hpp>
#include <boost/log/log.hpp>

#include "temsimexception.hpp"
#include "debugging.hpp"
#include "logging.hpp"
#include "rule.hpp"
#include "inifile.hpp"
#include "timeseries.hpp"

using std::string;
using std::map;
using std::multimap;
using std::vector;
using std::istream;

using boost::format;

/// declare the boost logging stuff.
BOOST_DECLARE_LOG(objectregister)

class Simulation;
class FileSystem;

/**
\file
The Object Register module provides a mapping from strings to classes and
class members to support run-time creation and modification of objects.

The Simulation class uses an ObjectRegister and ObjectFactory to create the main
model objects (storages, power stations and so on.)  The Bookkeeping module
uses the same object register to find object methods to retirev data for
reporting purposes.

In general, all objects that could require some kind of run-time information
about other objects could make use of the Object Register module.

C++ does not inherently provide run-time code evaluation; nor does it provide
reflection capabilities. However, there is no barrier to implementing these
capabilities within standard C++. C++ does provide limited Run-Time Type
Information (RTTI) via the typeid() operator and the type_info class.

\section sec_example Examples of Use

Simple example of use:
@code
// create an object register
Register reg;

// a simple variable to test our register
int x = 123;

// store the address of x in the object register, with the identifier "x"
reg.Set("x", &x);

// a pointer-to-int for us to retrieve "x" into
int * px = 0;

// retrieve the object (in this case a pointer-to-int) with the id "x"
reg.Get("x", px);

// now we can be sure that px points to x
assert(px == &x);
@endcode

We also use the Object Register to provide runtime retrieval of specific
objects, identified by their class and instance name. For a class to be
accessible in this way, it needs to provide the following code:
@code
    static string class_name; // eg. "Storage"
    virtual const string& ClassName() { return classname; }
    string instance_name; // eg. "Great_Lake"
    virtual const string& Name() { return instance_name; }
    virtual const string& InstanceName() { return instance_name; }
    // shared pointer type
    typedef boost::shared_ptr<Storage> Ptr;
    // constructor taking an instance name ??
    Storage(const std::string& aName) : instance_name(aName) {}
@endcode
With this code in place, we can do this:
@code
Storage::Ptr object(new Storage("Great_Lake"));
reg.SetInstance(object);
@endcode
This will register the smart pointer with the identifier "Storage.Great_Lake"
and call the object->Register(reg) method to register the object's member
variables as well.

We can retrieve a class instance registered in this way by using GetInstance:
@code
Storage::Ptr p;
reg.FindInstance("Great_Lake", p);
@endcode

An example implementation of the class's Register method
might look like this:
@code
// assume the following member variables
// double mEOL;
// Channel::Ptr mpSpillOutlet;
// std::vector<Channel::Ptr> mSources;

void Storage::Register(ObjectRegister& aReg){
    aReg.Set(*this, "EOL", &mEOL);
    aReg.Set(*this, "SpillOutlet", &mpSpillOutlet);
    aReg.Set(*this, "Sources", &mSources);
}
@endcode
Note that we register <b>pointers</b> to the member variables.

Any variable that is registered in this way (as a pointer) can have an
associated "reset" value stored as a string:
@code
reg.SetString("Storage.Great_Lake.EOL", "123.4");
reg.SetString("Storage.Great_Lake.Spill", "spillway");
reg.SetString("Storage.Great_Lake.Sources", "[mersey, forth]");
@endcode
When ObjectRegister::Reset() is called, the values represented by the strings
are transferred to the values pointed to by the corresponding entry in the
object register. If the entry in the object register is a (pointer to a)
boost::shared_pointer (as in "SpillOutlet" in our example,) then the string is
assumed to be the instance name of an object of the appropriate type. So in the
above example, we require that there be Channel objects with the instance_names
of "spillway", "mersey", and "forth". We also note that it is possible to have
std::vector collections of objects, where the string representation is a comma
-separated list enclosed by brackets. If the thing pointed to is not a
shared_pointer, it is required to be convertable from a string via
boost::lexical_cast.

\section sec_callbacks Callbacks

The ObjectRegister also has the ability to have named callbacks of two types
added to the object register. Callbacks are boost::function objects. The first
type is a boost::funtion<void (void)> and the second type is
boost::function<void (const DateTime&)>. Multiple callbacks can be added with
the same identifying name so they can be called as a group. For example:

@code
reg.AddTimeCallback(
    "Initialise", // name of our callback collection
    boost::bind(
        &MyClass::Init, // MyClass::Init(const DateTime& arTime)
        &some_instance  // of MyClass
    )
)
@endcode

Later, this callback can be triggered by:
@code
DateTime t = DateTime(2005,1,1);
reg.DoTimeCallbacks("Initialise", t);
@endcode

\section sec_obj_factory Object Factory

We can use the ObjectFactory class to make objects of a given type. First we
create an object factory and tell it which object register to use:
@code
    ObjectFactory factory;
    factory.SetRegister(&reg);
@endcode
We then need to create some "Maker" objects for all the classes we with to make.
These Maker abjects are added to the object factory.
@code
    factory.AddMaker("Storage", &MakeObject<Storage>);
    factory.AddMaker("Channel", &MakeObject<Channel>);
    factory.AddMaker("PowerStation", &MakeObject<PowerStation>);
@endcode
We can create a map of string data for the object to use (as per the SetString
and Reset methods above.)
@code
    map<string, string> data;
    data["EOL"] = "123.4";
    data["Spill"] = "spillway";
    data["Sources"] = "[mersey, forth]";
@endcode
We then call the Make method giving it the class of object to create, an
instance name, and the string data.
@code
    factory.Make("Storage", "Gordon", data);
    reg.Reset();
@endcode

\section sec_bookkeeping Bookkeeping

For bookkeeping and reporting purposes, objects can register member functions
that are of type "double (void)". These functions are registered with the
identifier "class::function"; eg. for the function "double Volume()" in the
Storage class, it would be registered as "Storage::Volume". If Lake_Gordon is an
instance of the Storage class, then a data specifier like "Lake_gordon.Volume(hourly)"
would cause the bookkeeping classes to call the Volume member on the Lake_Gordon 
object.

*/

/// separator for parts of the register string.
extern const char* gRegStringSeps;

/// prefix for functions being stored in the register
extern const char* gRegStringFunction;

/// prefix for collections being stored in the register
extern const char* gRegStringCollection;

/// prefix for files being stored in the register
extern const char* gRegStringFile;

/// make a register string from 1, 2, or 3 components
std::string RegisterString( const std::string& aFirst,
                            const std::string& aSecond="",
                            const std::string& aThird="");

/// Make a register string for a member of a class. We require that there are
/// member functions called ClassName() and Name().
template <typename T>
std::string MemberRegisterString(   T& aObject,
                                    const std::string& aVarName="") {
    return RegisterString(  aObject.ClassName(),
                            aObject.Name(),
                            aVarName );
}

/// Construct a register string for a function.
std::string FunctionRegisterString(const std::string& aFunctionName);

/// Construct a register string for a file.
std::string FileRegisterString(const std::string& aFileName);

/// Construct a register string for a collection.
std::string CollectionRegisterString(const std::string& aCollectionName);


class ObjectRegister; // forward decl.

//---------------------------------------------------------------------
// Object register implementation:
// We have templated classes and functions so we can avoid specifying
// all the types somewhere.
// We also make use of the type_info class and typeid operator as
// map keys.

/// Dummy base class so we can use derived templated classes for each type.
class BaseRegister {
public:
    /// shared ptr for BaseRegister
    typedef boost::shared_ptr<BaseRegister> Ptr;

    /** Set the string representation of the variable pointed to in the type
    register. */
    virtual void GetString(const string aKey, string& aString) {}

    /** Set the string representation of the variable pointed to in the type
    register. */
    virtual void SetString(const string aKey, const string aString) {}

    /** Set the value of the thing pointed to by the object register entry from
    the string representation */
    virtual void Reset(ObjectRegister&) {}

};

/// Our true Type register - template ensures we get one for each type.
template <typename T>
class Register : public BaseRegister {
public:
    virtual ~Register() {}

    /// We store our T here, indexed by strings
    map<string, T> Data;
    /// We store an optional string representation here
    map<string, string> StringData;

    /// typedef for a shared pointer to Register<T>.
    typedef boost::shared_ptr<Register< T > > Ptr;

    /// retrieve a value of type T from our collection
    /// @param aKey The string identifier for the value
    /// @param aVal A ref var to receive the retrieved value
    void Get(const string aKey, T& aVal) {
        typename map<string, T>::const_iterator val_it = Data.find(aKey);

        if (val_it == Data.end()) {
            throw TemsimException(boost::str(boost::format(
                    "Couldn't find key [%d] in object register")
                    % aKey).c_str());
        }
        else {
            // set our output to the pointer retrieved from the map
            aVal = (*val_it).second;
        }
    }

    /// retrieve the string representation of a value of type T 
    /// from our collection
    /// @param aKey The string identifier for the value
    /// @param aString A ref var to receive the retrieved string
    void GetString(const string aKey, string& aString) {
        typename map<string, string>::const_iterator
            str_it = StringData.find(aKey);

        if (str_it == StringData.end()) {
            throw TemsimException(boost::str(boost::format(
                    "Couldn't find key [%d] in object register")
                    % aKey).c_str());
        }
        else {
            // set our output to the pointer retrieved from the map
            aString = (*str_it).second;
        }
    }

    /// store a value in our collection
    /// @param aKey String id for the value stored.
    /// @param aVal Ref to the value to be stored. The value is stored "by
    /// value", ie. a copy of aVal is made.
    /// @param apDefaultValue Default value as a string
    void Set(const string aKey, T aVal, const char* apDefaultValue = NULL) {
        Data[aKey] = aVal;
        if (apDefaultValue != NULL) {
            SetString(aKey, string(apDefaultValue));
        }
    }

    /// Set the string repsentation of a value with the given key.
    /// @param aKey The string id of the value stored.
    /// @param aString The string representation of the value, eg. "1.23" for
    /// a floating point value. If the value stored is of type shared_ptr<T>
    /// then the string representation is assumed to be the instance name of
    /// an object of type T.
    virtual void SetString(const string aKey, const string aString) {
        StringData[aKey] = aString;
    }

    /** Set the value of the thing stored in the register from the string
    representation */
    void Reset(ObjectRegister& aReg) {
        // for each of our StringData entries, we call ResetFromString
        for (map<string, string>::const_iterator string_data
                = StringData.begin();
            string_data != StringData.end();
            ++string_data) {

            string s = (*string_data).second;
            // find the Data value with the same id.
            T p = Data[(*string_data).first];
            BOOST_LOGL(objectregister,info) << "Reset: " << s << " -> " << string_data->first << std::endl;
            ResetFromString(aReg, p, s);
        }
    }

};

/// Our overall Register class that keeps a collection of the type-specific
/// type registers.
class ObjectRegister {
public:
    /// Constructor.
    ObjectRegister() : mpSimulation(NULL) {}

    /// Check to see if a given var name is okay.
    /// a valid variable is of the form:
    /// (alpha) (alphanum | '_')*
    static bool IsValidVariableName(string& arName);

    /// This is where we keep our type-specific registers.
    map<string, BaseRegister::Ptr> Registers;

    /** We store a map from Ids to type names so we can deal with SetString
    (since SetString doesn't have any type info, only the object's Id)  */
    map<string, string> TypeNames;

    /// Clear the object register of its registers
    /// and typenames.
    void Clear() { Registers.clear(); TypeNames.clear(); }

    /// Get the type of a string identifier.
    /// @param aKey The string identifier for the type.
    /// @returns the type of the identifier as a string.
    string GetType(string aKey) { return TypeNames[aKey]; }

    /// Getting a value from the appropriate type register
    /// @param aKey The string identifier for the value
    /// @return The value corresponding to the key argument
    template <typename T>
    T   Get(const string aKey) {
        T temp;
        Get(aKey, temp);
        return temp;
    }

    /// Register a class member with the given name in the class.
    /// @param arObj the object whose member variable we wish to register.
    /// @param arKey the identifier to which the variable is assigned.
    /// @param arVal the location that we store the value in.
    /// @param apDefaultValue the default value, or NULL.
    template <typename S, typename T>
    void RegisterMember(S& arObj, const string& arKey, const T& arVal, const char* apDefaultValue = NULL) {
        Set(arObj, arKey, arVal, apDefaultValue);
    }

    /// Test to see if a given key exists in the register
    /// @param aKey The string key we are looking for
    /// @returns True if the key exists, false otherwise
    bool HasKey(const string& aKey) {
        map<string, string>::const_iterator iter = TypeNames.find(aKey);
        if (iter == TypeNames.end()) {
            return false;
        }
        else {
            return true;
        }
    }

    /// Getting a value from the appropriate type register
    /// @param aKey The string identifier for the value
    /// @param aVal A ref var to receive the retrieved value
    template <typename T>
    void Get(const string aKey, T& aVal) {
        typename Register<T>::Ptr pRegT;

        // check to see we have a type register for that type
        map<string, BaseRegister::Ptr>::const_iterator reg_it
            = Registers.find(typeid(aVal).name());

        if (reg_it != Registers.end()) {
            // we do have a type register
            // This cast is always okay because we know that the BaseReg* stored
            // in Regs and indexed by typeid(T).name() *must* be a Reg<T>*
            // as we created it and put it there.

            pRegT = boost::static_pointer_cast< Register<T> >
                    ( (*reg_it).second );
            // get the var from the type register.

            pRegT->Get(aKey, aVal);
        }
        else {
            // something is wrong, we dont have one of those type registers!
            throw TemsimException(boost::str(boost::format(
                    "Couldn't find object register for type [%d]")
                    % typeid(aVal).name()).c_str());
        }
    }

    /// Getting a string representation of a value from the 
    /// appropriate type register
    /// @param aKey The string identifier for the value
    /// @param aString A ref var to receive the retrieved string
    void GetString(const string aKey, string& aString) {
        BaseRegister::Ptr pRegT;
        string type_name;
        // check to see if we have a type name for this key
        map<string, string>::iterator reg_it
            = TypeNames.find(aKey);

        if (reg_it == TypeNames.end()) {
            throw TemsimException("Couldn't find a typename for key " + aKey, 
                "ObjectRegister");
        } else {    
            // we have the appropriate type name
            type_name = (*reg_it).second;
            pRegT = Registers[type_name];
            pRegT->GetString(aKey, aString);
        }
    }

    /// Get an instance (shared_ptr) with the given instance name
    /// @param aName The instance name, eg. "Great_Lake"
    /// @param aPtr Ref to a shared_ptr<T> to receive the value of the class
    /// instance of type T stored with id "T::class_name.aName"
    template <typename T>
    void FindInstance(const string aName, boost::shared_ptr<T>& aPtr) {
        string key = RegisterString(T::class_name, aName);
        Get(key, aPtr);
    }

    /// Storing a value in the appropriate type register
    /// @param arKey String id for the value stored.
    /// @param arVal Ref to the value to be stored. The value is stored "by
    /// @param apDefaultValue Default value of the object as a string
    /// value", ie. a copy of aVal is made.
    template <typename T>
    void Set(const string& arKey, const T& arVal, const char* apDefaultValue = NULL) {
        BaseRegister::Ptr pRegT;
        // check to see if we have a type register of this type
        map<string, BaseRegister::Ptr>::iterator reg_it
            = Registers.find(typeid(arVal).name());

        if (reg_it == Registers.end()) {
            // we need a new type register for this type
            pRegT.reset(new Register<T>);
            Registers[typeid(arVal).name()] = pRegT;
        }
        else {
            // we have the appropriate type register
            pRegT = (*reg_it).second;
        }

        // set the var in the type register
        typename Register<T>::Ptr TRegPtr;
        // this static_cast is always okay as we either created the object
        // ourselves (a new type register) or we retrieved it based on the
        // type info. Either way, we know it is of the correct type.
        TRegPtr = boost::static_pointer_cast<Register<T> >(pRegT);
        TRegPtr->Set(arKey, arVal, apDefaultValue);

        TypeNames[arKey] = typeid(arVal).name();
    }

    /** Store an instance (ie. a shared_ptr) in the register and then call the
    instance's Register method.
    @param aPtr Ref to a shared_ptr to an object to store. The id is constructed
    to from the object's class_name and instance_name values.
    */
    template <typename T>
    void SetInstance(const boost::shared_ptr<T>& aPtr) {
        string key = RegisterString(T::class_name, aPtr->Name());
        Set(key, aPtr);
        aPtr->Register(*GetSimulation());
    }

    /// Store the string represenation of a value
    /// @param aKey The string id of the value stored.
    /// @param aString The string representation of the value, eg. "1.23" for
    /// a floating point value. If the value stored is of type shared_ptr<T>
    /// then the string representation is assumed to be the instance name of
    /// an object of type T.
    void SetString(const string& aKey, const string& aString) {
        BaseRegister::Ptr pRegT;
        string type_name;
        // check to see if we have a type name for this key
        map<string, string>::iterator reg_it
            = TypeNames.find(aKey);

        if (reg_it == TypeNames.end()) {
            throw TemsimException("Couldn't find a typename for key " + aKey,
                "ObjectRegister");
        } else {
            // we have the appropriate type name
            type_name = (*reg_it).second;
            pRegT = Registers[type_name];
            pRegT->SetString(aKey, aString);
        }
    }

    /** Set a value in the register - used for member variables, as the first
    arg is an object from which we get class and instance name.
    @param arObj Ref to object from which we get the class_name.
    @param arKey String identifier for the value to store - the full id is
    constructed from aObj's class_name and isntance name, plus this parameter.
    @param arVal Ref to the value to store (usually a pointer to a member
    @param apDefaultValue Default value for the object as a string
    variable.)
    */
    template <typename S, typename T>
    void Set(S& arObj, const string& arKey, const T& arVal, const char* apDefaultValue = NULL) {
        string key = RegisterString(S::class_name, arObj.Name(), arKey);
        Set(key, arVal, apDefaultValue);
    }

    /** Call Reset() on each of the specific type registers, to set the stored
    values from any string representations that might be present. */
    void Reset() {
        // for each of our Register entries, we call Reset
        for (map<string, BaseRegister::Ptr>::const_iterator regs
                = Registers.begin();
            regs != Registers.end();
            ++regs) {

            (*regs).second->Reset(*this);
        }
    }

    //--------------------------------------
    // callback implemenation

    /// collection of callbacks that take no argument
    multimap<string, boost::function<void (void)> >
        mVoidCallbacks;

    /// collection of callbacks that take a const DateTime& argument
    multimap<string, boost::function<void (const DateTime&)> >
        mTimeCallbacks;

    /// add a callback that takes no arguments to the collection with the
    /// specified name
    void AddVoidCallback(string aName, boost::function<void (void)> aFunctor) {
        typedef
            multimap<string, boost::function<void (void)> >::value_type
            valType;
        mVoidCallbacks.insert( valType(aName, aFunctor) );
    }

    /// add a callback that takes a const DateTime& argument to the collection
    /// with the specified name
    void AddTimeCallback(string aName,
                        boost::function<void (const DateTime&)> aFunctor) {
        typedef
            multimap<string,
                    boost::function<void (const DateTime&)>
                    >::value_type
            valType;
        mTimeCallbacks.insert( valType(aName, aFunctor) );
    }

    /// call the callbacks in the collection with the specified name
    void DoVoidCallbacks(string aName) {
        BOOST_LOGL(objectregister,info) << "VoidCallbacks " << aName << "..." << std::endl;
        typedef
            multimap<string,boost::function<void (void)> >::iterator
            iterator;
        std::pair<iterator, iterator> range;
        range = mVoidCallbacks.equal_range(aName);
        for ( ; range.first != range.second; range.first++ ) {
            ((*(range.first)).second)();
        }
        //BOOST_LOGL(objectregister,info) << "Done!\n";
    }

    /// call the callbacks in the collection with the specified name
    void DoTimeCallbacks(string aName, const DateTime& arTime) {
        typedef
            multimap<string,boost::function<void (const DateTime&)> >::iterator
            iterator;
        std::pair<iterator, iterator> range;
        range = mTimeCallbacks.equal_range(aName);
        for ( ; range.first != range.second; range.first++ ) {
            ((*(range.first)).second)(arTime);
        }
    }

    /// Get the simulation associated with this object register
    Simulation* GetSimulation() { return mpSimulation; }
    /// Set the simulation assosciated with this object register
    void SetSimulation(Simulation* apSim) { mpSimulation = apSim; }

protected:
    /// The Simulation associated withthis object register
    Simulation* mpSimulation;

};

/// Set the value of a pointer to function from a string rep.
/// This function is required in order for duck-typing in the
/// Register class to work when it calls Reset().
template <typename T>
void ResetFromString(ObjectRegister&, double (T::*pf)(void), string s) {
    /// @todo Should throw an error here
}

/// Set the value of a pointer to function from a string rep.
/// This function is required in order for duck-typing in the
/// Register class to work when it calls Reset().
template <typename T>
void ResetFromString(ObjectRegister&, double (T::*pf)(void) const, string s) {
    /// @todo Should throw an error here
}

/** Set a value, via a pointer, from a string representation. Assumes that the
type T can be converted from a string via boost::lexical_cast
@param arReg Ref to an object register - not needed in this case.
@param p Pointer to the value to set.
@param s String representation of the value.
*/
template <typename T>
void ResetFromString(ObjectRegister& arReg, T* p, string s) {
    try {
        *p = boost::lexical_cast<T>(s);
    }  catch (...) {
        throw TemsimException("Couldn't reset value to " + s, "ObjectRegister");
    }    
}

/** partial template specialization for booleans, seeing as lexical cast
can't deal with them. **/
template<>
inline void ResetFromString(ObjectRegister&, bool* p, string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "true" || s == "yes" || s == "y") {
        *p = true;
    } else if (s == "false" || s == "no" || s == "n") {
        *p = false;
    } else {
        throw TemsimException("Couldn't reset value to " + s, "ObjectRegister");
    }
}

/** set a datetime value, via a pointer, from a string representation.
**/
template <>
inline void ResetFromString(ObjectRegister& arReg, DateTime* p, string s) {
    *p = DateTime::FromString(s, arReg.GetSimulation());
}

/** Set a shared_ptr from the instance name identifying a particular instance
that is also stored as a shared_ptr in the object register
@param arReg Ref to an object register.
@param p Ref to a shared_ptr to set.
@param s Instance name of the object whose shared_ptr we want.
*/
template <typename T>
void ResetFromString(ObjectRegister& arReg, boost::shared_ptr<T>& p, string s) {
    arReg.FindInstance(s, p);
}

/** Set a shared_ptr from the instance name identifying a particular instance
that is also stored as a shared_ptr in the object register
@param arReg Ref to an object register.
@param p Pointer to a shared_ptr to set.
@param s Instance name of the object whose shared_ptr we want.
*/
template <typename T>
void ResetFromString(ObjectRegister& arReg, boost::shared_ptr<T>* p, string s) {
    boost::shared_ptr<T> temp;
    arReg.FindInstance(s, temp);
    *p = temp;
}

/** Set a vector of values from a string representation.
@param arReg Ref to an object register.
@param v Pointer to a vector of values we want to set.
@param s String representation of the values in the vector, eg "[1.23, 4.56]"
for a vector of floats.
*/
template <typename T>
void ResetFromString(ObjectRegister& arReg, vector<T>* v, string s) {
    v->clear();
    boost::char_separator<char> sep(EnhancedIniFile::sSeps);
    boost::tokenizer<boost::char_separator<char> > tok(s, sep);
    for(boost::tokenizer<boost::char_separator<char> >::iterator
            beg = tok.begin();
        beg != tok.end();
        ++beg ) {

        T temp;
        ResetFromString(arReg, &temp, *beg);
        v->push_back(temp);
    }
}

/** Object Factory helper function. Takes a class name, instance name, an object
register, and a collection of member variable data as strings. Need to
instantiate this function for each type that the factory can make.
@param aClassName String class name for the object to create.
@param aName String instance name to give the object.
@param apReg Pointer to an object register.
@param aInifile map<string, string> describing the "inifile" data (name-value
pairs) to initialise the object.
*/
template <typename T>
void
MakeObject( const string& aClassName,
            const string& aName,
            ObjectRegister* apReg,
            map<string, string>& aInifile) {

    // make a new object of type T and store it in a shared_ptr
    typename T::Ptr thing(new T(aName));

    // register the object somewhere
    apReg->SetInstance(thing);

    // go through the NameValue pairs and set each string value in the object
    // register

    for (map<string, string>::const_iterator nv = aInifile.begin();
        nv != aInifile.end();
        ++nv) {

        string var_name = (*nv).first;
        string var_value = (*nv).second;
        try {
            apReg->SetString(RegisterString(aClassName,
                aName, var_name), var_value);
        } catch(TemsimException& e) {
            throw TemsimException("member '" 
                + var_name + "' not defined for " + aClassName + ": " + e.what());
        }
    }

};

/// typedef of a function that matches the signature of MakeObject above.
typedef
boost::function<void (  const string&,
                        const string&,
                        ObjectRegister*,
                        map<string, string>& )> Maker;

/**
This class is used to make objects of a given type, register them in an object
register, and set their member variable string data.
*/
class ObjectFactory {
public:

    /// Set the object register pointer for this factory
    void SetRegister(ObjectRegister* apReg) {
        mpRegister = apReg;
    }

    /** Make an object of the given class and instance name using the supplied
    string data for member variables.
    @param arClassName String class name for the object to create.
    @param arName String instance name to give the object.
    @param arInifile map<string, string> describing the "inifile" data
    (name-value pairs.)
    */
    void
    Make(   const string& arClassName,
            const string& arName,
            map<string, string>& arInifile ) {

        map<string, Maker>::iterator iter = mMakers.find(arClassName);
        if (iter != mMakers.end()) {
            (mMakers[arClassName])(arClassName, arName, mpRegister, arInifile);
        } else {
            throw TemsimException(string("Class '") + arClassName + 
                string("' not registered for use in T4 INI file"), "ObjectRegister");
        }
    }

    /** Add a "Maker" function to the factory
    @param arClassName Name of the class whose maker function we are adding.
    @param aMaker A particular "Maker" fucntion (eg. &MakeObject<T>) for the
    specified class.
    @param arBaseClass Name of this object's base class
    */
    void AddMaker(const string& arClassName, Maker aMaker, const string& arBaseClass = "") {
        mMakers[arClassName] = aMaker;
        if (arBaseClass != "") {
            mClassRelationships[arClassName] = arBaseClass;
        }
    }

    /// Add a Maker function to the factory, given only an object type
    /// (The type must have a class_name member.)
    template <typename T>
    void AddMaker() {
        AddMaker(T::class_name, &MakeObject<T>);
    }

    /// Add a Maker to the factory, given the object's type and base type.
    /// Both subtype and basetype must have class_name members.
    template <typename subType, typename baseType>
    void AddMaker() {
        AddMaker(subType::class_name, &MakeObject<subType>, baseType::class_name);
    }

protected:
    map<string, Maker>  mMakers;             ///< collection of Maker objects.
    map<string, string> mClassRelationships; ///< mapping of subclass to base class.
    ObjectRegister*     mpRegister;          ///< object register for this factory
};

/** Utility function to read an istream that contains object data in the
"enhanced ini file" format, and make the appropriate objects as described by the
ini file data. */
void MakeObjectsFromIniFile(ObjectFactory& arFactory, istream* apStream, 
                            string aStreamName, FileSystem& arFileSystem,
                            ObjectRegister& arRegister);


#endif



