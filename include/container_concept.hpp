#pragma once
#include <concepts>
#include <iterator>

template <typename T>
concept ContainerConcept = requires(T t, const T ct) {
    typename T::value_type;
    { t.begin() } -> std::same_as<typename T::iterator>;
    { t.end() } -> std::same_as<typename T::iterator>;
    { ct.begin() } -> std::same_as<typename T::const_iterator>;
    { ct.end() } -> std::same_as<typename T::const_iterator>;
    { t[0] };
    { ct[0] };
};

template <typename T>
concept PushableContainerConcept =
    ContainerConcept<T> &&
    requires(T t, typename T::value_type val, typename T::iterator it) {
        { t.push_back(val) };
        { t.insert(it, it, it) };
    };