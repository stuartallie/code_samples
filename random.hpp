#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <boost/random.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/log/log.hpp>

#include "logging.hpp"

/// declare the boost logging stuff.
BOOST_DECLARE_LOG(randomnumbergenerator)

/**
\file
Module for a range of Random Number Generators.
*/


class Simulation;

/**
A base class for all Random Number Generators (RNGs)
*/
class BaseRNG {
public:
    /// shared pointer for the BaseRNG class
    typedef boost::shared_ptr<BaseRNG> Ptr;

    /// Seed the RNG
    /// @param aSeed seed value for the RNG
    virtual void Seed(int aSeed)=0;
};


/** 
Uniformly distributed integer RNG
*/
template <typename IntType=int>
class UniformIntRNG : public BaseRNG {
public:
    /// Integral type for the result of the RNG.
    typedef IntType ResultType;
    
    /// shared pointer to the UniformIntRNG class
    typedef boost::shared_ptr<UniformIntRNG<IntType> > Ptr;

    /** Construct a UniformIntRNG that generates integers in the range 
    [aMin, aMax] with the initial seed aSeed.
    @param aMin Minimum value for the result of the RNG.
    @param aMax Maximum value for the result of the RNG.
    @param aSeed Initial seed value.
    */    
    UniformIntRNG(IntType aMin, IntType aMax, IntType aSeed = 1);

    virtual ~UniformIntRNG();

    /// Return the next random number in the sequence.    
    IntType operator()();
    
    /// Seed the contained random numner source.
    /// @param aSeed seed value for the RNG
    void Seed(int aSeed);

private:
    boost::mt19937                  mRNG;   ///< the random source
    boost::uniform_int<IntType>     mDistribution; ///< uniform distribution
    boost::variate_generator<
        boost::mt19937, 
        boost::uniform_int< IntType > 
    >*  mpVarGen;   ///< The link between the random number and the distibution.

};

// constructor
template <typename T>
UniformIntRNG<T>::UniformIntRNG(T aMin, T aMax, T aSeed) {
    mRNG.seed((boost::mt19937::result_type)aSeed);
    mDistribution = boost::uniform_int<T>(aMin, aMax);
    mpVarGen = new
        boost::variate_generator<boost::mt19937, 
                                boost::uniform_int<T> >(mRNG,
                                                        mDistribution);
};

// destructor - deletes the variate_generator object created in the constructor
template <typename T>
UniformIntRNG<T>::~UniformIntRNG() {
    if (mpVarGen) { 
        delete mpVarGen;
        mpVarGen = 0;
    }
}

// returns the next random value
template <typename T>
T UniformIntRNG<T>::operator()() {
    return (*mpVarGen)();
}

// seed the RNG
template <typename T>
void UniformIntRNG<T>::Seed(int aSeed) {
    BOOST_LOGL(randomnumbergenerator, info) << "Seeding with value " 
            << aSeed << std::endl;
    mpVarGen->engine().seed((boost::mt19937::result_type)aSeed);
}

/** 
Uniformly distributed floating point RNG
*/
template <typename FloatType=double>
class UniformFloatRNG : public BaseRNG {
public:
    /// Integral type for the result of the RNG.
    typedef FloatType ResultType;
    
    /// shared pointer to the UniformFloatRNG class
    typedef boost::shared_ptr<UniformFloatRNG<FloatType> > Ptr;

    /** Construct a UniformFloatRNG that generates integers in the range 
    [aMin, aMax] with the initial seed aSeed.
    @param aMin Minimum value for the result of the RNG.
    @param aMax Maximum value for the result of the RNG.
    @param aSeed Initial seed value.
    */    
    UniformFloatRNG(FloatType aMin, FloatType aMax, int aSeed = 1);

    virtual ~UniformFloatRNG();

    /// Return the next random number in the sequence.    
    FloatType operator()();
    
    /// Seed the contained random numner source.
    /// @param aSeed seed value for the RNG
    void Seed(int aSeed);

private:
    boost::mt19937                  mRNG;   ///< the random source
    boost::uniform_real<FloatType>     mDistribution; ///< uniform distribution
    boost::variate_generator<
        boost::mt19937, 
        boost::uniform_real< FloatType > 
    >*  mpVarGen;   ///< The link between the random number and the distibution.

};

// constructor
template <typename T>
UniformFloatRNG<T>::UniformFloatRNG(T aMin, T aMax, int aSeed) {
    mRNG.seed((boost::mt19937::result_type)aSeed);
    mDistribution = boost::uniform_real<T>(aMin, aMax);
    mpVarGen = new
        boost::variate_generator<boost::mt19937, 
                                boost::uniform_real<T> >(mRNG,
                                                        mDistribution);
};

// destructor - deletes the variate_generator object created in the constructor
template <typename T>
UniformFloatRNG<T>::~UniformFloatRNG() {
    if (mpVarGen) { 
        delete mpVarGen;
        mpVarGen = 0;
    }
}

// returns the next random value
template <typename T>
T UniformFloatRNG<T>::operator()() {
    return (*mpVarGen)();
}

// seed the RNG
template <typename T>
void UniformFloatRNG<T>::Seed(int aSeed) {
    BOOST_LOGL(randomnumbergenerator, info) << "Seeding with value " 
            << aSeed << std::endl;
    mpVarGen->engine().seed((boost::mt19937::result_type)aSeed);
}

/**
Normall (gaussian) distributed floating point values.
*/
template <typename T=double>
class NormalRNG : public BaseRNG {
public:
    /// float-type for the result value of the RNG
    typedef T ResultType;
    
    /// shared pointer  for the NormalRNG class
    typedef boost::shared_ptr<NormalRNG< T > > Ptr;

    /** Construct a NormalRNG with average value aMean, standard deviation aStd
    and the initial seed value aSeed.
    @param aMean Mean of the normal distribution.
    @param aStd Standard deviation of the normal distribution.
    @param aSeed Initial seed value for the RNG.
    */
    NormalRNG(T aMean, T aStd, int aSeed = 1);

    virtual ~NormalRNG();
    
    /// returns the next random value
    T operator()();
    
    /// Seed the contained random numner source.
    /// @param aSeed seed value for the RNG
    void Seed(int aSeed);

private:
    boost::mt19937                  mRNG;           ///< the random source
    boost::normal_distribution<T>   mDistribution;  ///< normal distribution
    boost::variate_generator<
        boost::mt19937, 
        boost::normal_distribution< T > 
    >*  mpVarGen;   ///< link between random source and distribution

};

// constructor
template <typename T>
NormalRNG<T>::NormalRNG(T aMean, T aStd, int aSeed) {
    mRNG.seed((boost::mt19937::result_type)aSeed);
    mDistribution = boost::normal_distribution<T>(aMean, aStd);
    mpVarGen = 
        new boost::variate_generator<
                boost::mt19937,
                boost::normal_distribution< T > >(  mRNG,
                                                    mDistribution);
};

// destructor
template <typename T>
NormalRNG<T>::~NormalRNG() {
    if (mpVarGen) { 
        delete mpVarGen;
        mpVarGen = 0;
    }
}

// returns next random value
template <typename T>
T NormalRNG<T>::operator()() {
    return (*mpVarGen)();
}

// seed the random number source
template <typename T>
void NormalRNG<T>::Seed(int aSeed) {
    BOOST_LOGL(randomnumbergenerator, info) << "Seeding with value " 
            << aSeed << std::endl;
    mpVarGen->engine().seed((boost::mt19937::result_type)aSeed);
}


/**
A Class that encapsulates a UniformFloatRNG<double> as an object
available to the scripting environment.
*/
class RandomDouble {
public:
    // standard stuff for object register -----------------------------
    static string class_name; // "Pump"
    virtual const string& ClassName() const { return class_name; } 
    string instance_name; // eg. "Turbine1"
    const string& Name() { return instance_name; } 
    
    typedef boost::shared_ptr<RandomDouble> Ptr;

    // constructor taking an instance name.
    // @param arName the name of the instance.
    RandomDouble(const string& arName="default");
    
    // Register the object's members.
    // @param arSim The simulation to register
    // the class instance with.
    virtual void Register(Simulation& arSim);

    double Value();


    // -----------------------------------------------------------------

protected:
    UniformFloatRNG<double> mRNG;

};


/**
A Class that encapsulates a NormalRNG<double> as an object
available to the scripting environment.
*/
class RandomNormal {
public:
    // standard stuff for object register -----------------------------
    static string class_name; // "Pump"
    virtual const string& ClassName() const { return class_name; } 
    string instance_name; // eg. "Turbine1"
    const string& Name() { return instance_name; } 
    
    typedef boost::shared_ptr<RandomNormal> Ptr;

    // constructor taking an instance name.
    // @param arName the name of the instance.
    RandomNormal(const string& arName="default");
    
    // Register the object's members.
    // @param arSim The simulation to register
    // the class instance with.
    virtual void Register(Simulation& arSim);

    double Value();


    // -----------------------------------------------------------------

protected:
    NormalRNG<double> mRNG;

};


#endif


