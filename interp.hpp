#ifndef _INTERP_HPP_
#define _INTERP_HPP_

#include <map>

#include <boost/format.hpp>

#include "datetime.hpp"
#include "types.hpp"
#include "temsimexception.hpp"

using std::map;

using boost::format;
using boost::str;

/// Generic base class for interpolated values
template <typename KeyType, typename ValType>
class Interpolator {
public:
    typedef boost::shared_ptr<Interpolator<KeyType, ValType> > Ptr;
    typedef map<KeyType, ValType> TSMap;
    typedef typename TSMap::const_iterator TSMapIter;

    virtual ValType Value(const TSMap& arPoints, const KeyType& arKey) const=0;
};

/// Class implementing Linear Interpretation using map collection with general KeyType.
template <typename KeyType, typename ValType>
class LinearInterp: public Interpolator<KeyType, ValType> {

public:
    typedef boost::shared_ptr<LinearInterp<KeyType, ValType> > Ptr;

    /// Get the value in the timeseries at the specified time.
    /// @param arPoints the points to retrieve the value from.
    /// @param arKey The DateTime used to lookup a point within the 
    /// timeseries.
    /// @returns the timeseries value
    ValType Value(const map<KeyType, ValType>& arPoints, 
                  const KeyType& arKey) const {

        if (arPoints.size() <= 1) {
            // don't have enough points to interpolate
            throw TemsimException("Error evaluating time series - at most 1 point in time series.");
        }

        ValType value;
        TSMapIter lowerIter = arPoints.lower_bound(arKey);
        TSMapIter upperIter = lowerIter;
        if (lowerIter == arPoints.end()) {
            // key is past end of data, so extrapolate from last 2 values
            lowerIter--;
            upperIter = lowerIter;
            lowerIter--;
        }
        else if (lowerIter == arPoints.begin()) {
            // key is either before first point or equal to it
            // extrapolate from first 2 points
            upperIter = lowerIter;
            upperIter++;
        }
        else {
            // we are somewhere in the middle
            upperIter = lowerIter;
            lowerIter--;
        }

        if (lowerIter->first == arKey) {
            // actually on the exact key, so just return the value
            value = lowerIter->second;
        }
        else if (upperIter->first == arKey) {
            // actually on the exact key, so just return the value
            value = upperIter->second;
        }
        else {
            // not on the exact point, so need to interpolate/extrapolate
            value = CalcInterpolatedValue<KeyType, ValType>::Value(lowerIter, upperIter, arKey);
        }
        return value;
    }

    //ValType CalcInterpolatedValue(const TSMapIter& arLowerIter, const TSMapIter& arUpperIter, const KeyType& arKey);

};

/// Generic type for interpolating values from a (key,value) map
template <typename KeyType, typename ValType>
struct CalcInterpolatedValue {
    static ValType Value(typename const Interpolator<KeyType, ValType>::TSMapIter& arLowerIter, 
                typename const Interpolator<KeyType, ValType>::TSMapIter& arUpperIter, 
                const KeyType& arKey) {

        // perform interpolation/extrapolation given lower and upper points and key ("x") value
        return arLowerIter->second + ((arUpperIter->second - arLowerIter->second)
                * (arKey - arLowerIter->first) / 
                (arUpperIter->first - arLowerIter->first));
    }
};

// Specialised type for interpolating values from a (DateTime, value) map
template <typename ValType>
struct CalcInterpolatedValue<DateTime, ValType> {
    static ValType Value(typename const Interpolator<DateTime, ValType>::TSMapIter& arLowerIter, 
                typename const Interpolator<DateTime, ValType>::TSMapIter& arUpperIter, 
                const DateTime& arKey) {

        // perform interpolation/extrapolation given lower and upper points and key DateTime value
        // (DateTime-specific implementation)
        return arLowerIter->second + ((arUpperIter->second - arLowerIter->second)
                    * (arKey - arLowerIter->first).ticks() / 
                    (arUpperIter->first - arLowerIter->first).ticks());
    }
};

/// Class implementing Value in Next Interval TimeSeries type using map collection.
template <typename KeyType, typename ValType>
class NextIntervalInterp: public Interpolator<KeyType, ValType>  {
public:
    typedef boost::shared_ptr<NextIntervalInterp<KeyType, ValType> > Ptr;

    /// Get the value in the timeseries at the specified time.
    /// @param arPoints the points to retrieve the value from.
    /// @param arKey The key used to lookup a point within the 
    /// timeseries.
    /// @returns the value.
    ValType Value(const TSMap& arPoints, const KeyType& arKey) const {
        TSMapIter iter = arPoints.find(arKey);
        if (iter == arPoints.end()) { // DateTime does not exist in timeseries.
            iter = arPoints.upper_bound(arKey);
            if (iter == arPoints.begin()) {
                throw TemsimException(str(format("can't find date %s in series") 
                    % arKey));
            }           
            --iter;                    
        }
        return iter->second;
    }
};

/// Class implementing Value in Preceding Interval TimeSeries type using map collection.
template <typename KeyType, typename ValType>
class PrecedingIntervalInterp: public Interpolator<KeyType, ValType>  {
public:
    typedef boost::shared_ptr<PrecedingIntervalInterp<KeyType, ValType> > Ptr;

    /// Get the value in the timeseries at the specified time.
    /// @param arPoints the points to retrieve the value from.
    /// @param arKey The key used to lookup a point within the 
    /// timeseries.
    /// @returns the value.
    ValType Value(const TSMap& arPoints, const KeyType& arKey) const {
        TSMapIter iter = arPoints.find(arKey);
        if (iter == arPoints.end()) { // DateTime does not exist in timeseries.
            iter = arPoints.upper_bound(arKey);
            if (iter == arPoints.end()) {
                throw TemsimException(str(format("can't find date %s in series") 
                    % arKey));
            }            
        }
        return iter->second;
    }
};

#endif
