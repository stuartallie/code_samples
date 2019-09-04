#include "random.hpp"
#include "simulation.hpp"

/// define boost logging stuff.
BOOST_DEFINE_LOG(randomnumbergenerator, "randomnumbergenerator")

/**
\file
Implementation of Random Number Generators.
*/

string RandomDouble::class_name("RandomDouble");

RandomDouble::RandomDouble(const string& arName)
:   instance_name(arName),
    mRNG(0.0, 1.0)
{
}

void RandomDouble::Register(Simulation& arSim) {
    // make this object available for scripting
    arSim.RegisterInstanceForScripting(this);

    ObjectRegister& reg(arSim.Objects());

    // seed the object with the replicate number at the start of
    // each rep
    Event::Ptr start_of_rep = arSim.PreDispatchEvents()->FindEvent("start_of_rep");
    start_of_rep->AddAction(MakeVoidAction(
        boost::bind(
            &UniformFloatRNG<>::Seed,
            boost::ref(mRNG),
            boost::ref(
                arSim.RepControl().RefCurrentRep()
            )
        )
    ));
}

// call the RNG and return the result
double RandomDouble::Value() {
    double result = mRNG();
    return result;
}


//////////////////////////////////////////////////////

string RandomNormal::class_name("RandomNormal");

RandomNormal::RandomNormal(const string& arName)
:   instance_name(arName),
    mRNG(0.0, 1.0)
{
}

void RandomNormal::Register(Simulation& arSim) {
    // make this object available for scripting
    arSim.RegisterInstanceForScripting(this);

    ObjectRegister& reg(arSim.Objects());

    // seed the object with the replicate number at the start of
    // each rep
    Event::Ptr start_of_rep = arSim.PreDispatchEvents()->FindEvent("start_of_rep");
    start_of_rep->AddAction(MakeVoidAction(
        boost::bind(
            &NormalRNG<>::Seed,
            boost::ref(mRNG),
            boost::ref(
                arSim.RepControl().RefCurrentRep()
            )
        )
    ));
}

// call the RNG and return the result
double RandomNormal::Value() {
    double result = mRNG();
    return result;
}

