/*
 * niepce - fwk/base/propertybag.cpp
 *
 * Copyright (C) 2011-2013 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FWK_PROPERTYBAG_HPP_
#define __FWK_PROPERTYBAG_HPP_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <boost/variant.hpp>

#include "fwk/base/date.hpp"

namespace fwk {

typedef uint32_t PropertyIndex;
/** The empty value */
typedef boost::blank EmptyValue;
typedef std::vector<std::string> StringArray;
/** EmptyValue will be the default type */
typedef boost::variant<EmptyValue, int, std::string, StringArray, Date> PropertyValue;

typedef std::set<PropertyIndex> PropertySet;

/** Return if the property value is empty */
bool is_empty(const PropertyValue & v);
/** Return if it is an integer */
bool is_integer(const PropertyValue & v);
bool is_string(const PropertyValue & v);
/** Return the integer value (or 0 if empty) */
int get_integer(const PropertyValue & v);
/** Return the string value */
const std::string & get_string(const PropertyValue & v);

/** a property bag
 * It is important that the values for PropertyIndex be properly name spaced
 * by the caller.
 */
class PropertyBag
{
public:
    typedef std::shared_ptr<PropertyBag> Ptr;
    typedef std::map<PropertyIndex, PropertyValue> _Map;
    typedef _Map::const_iterator const_iterator;
    typedef _Map::value_type value_type;

    bool empty() const
        {
            return m_bag.empty();
        }

    const_iterator begin() const
        {
            return m_bag.cbegin();
        }
    const_iterator end() const
        {
            return m_bag.cend();
        }
    const_iterator cbegin() const
        {
            return m_bag.cbegin();
        }
    const_iterator cend() const
        {
            return m_bag.cend();
        }


    /** return true if a property was removed prior to insertion */
    bool set_value_for_property(PropertyIndex idx, const PropertyValue & value);
    /** return true if a property is found */
    bool get_value_for_property(PropertyIndex idx, PropertyValue & value) const;
    /** return true if property exist */
    bool has_value_for_property(PropertyIndex idx) const;
    /** return true if the property was removed */
    bool remove_value_for_property(PropertyIndex idx);
private:
    _Map    m_bag;
};


}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
#endif
