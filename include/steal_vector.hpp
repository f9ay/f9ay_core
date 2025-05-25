#pragma once

#include <vector>

// from
// https://stackoverflow.com/questions/68349027/is-it-possible-to-extract-data-from-stdvector-without-copying-it-and-make-th
template <typename TElement>
TElement* the_pointer_heist(std::vector<TElement>& victim) {
    union Theft {
        std::vector<TElement> target;
        ~Theft() {}
    } place_for_crime = {std::move(victim)};
    return place_for_crime.target.data();
}