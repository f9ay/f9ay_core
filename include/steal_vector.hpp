#pragma once

#include <vector>

namespace f9ay {
// from
// https://stackoverflow.com/questions/68349027/is-it-possible-to-extract-data-from-stdvector-without-copying-it-and-make-th
template <typename T>
struct Stealer : public std::vector<T> {
    using std::vector<T>::vector;
    ~Stealer() = default;
};
template <typename TElement>
TElement* the_pointer_heist(std::vector<TElement>& victim) {
    Stealer stealer = std::move(victim);
    union Theft {
        std::vector<TElement> target;
        ~Theft() {}
    } place_for_crime = {std::move(victim)};
    return place_for_crime.target.data();
}
}  // namespace f9ay