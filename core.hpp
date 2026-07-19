#include <span>
#include <stdexcept>
#include <tuple>
#include <concepts>
#include <type_traits>
#include <utility>
#include "utils.hpp"

namespace miz {


template <typename ElementType, typename Mark>
struct type_mark {
    using element_type = ElementType;
    using mark_type = Mark;
    ElementType value;
};

template <typename Mark, typename ElementType>
constexpr type_mark<std::remove_cvref_t<ElementType>, Mark> tpm(ElementType&& et) {
    return { std::forward(et) };
}

template <typename... T> struct type_list {
    template <typename U>
    requires util::is_within<U, T...>
    using get_by_type = U;

    template <unsigned int I>
    using get_by_idx = std::tuple_element_t<I, std::tuple<T...>>;
};

namespace detail {
    template <typename Handler, typename T>
    concept Acceptable = requires(T obj) {
        Handler::accept(obj);
    };

    template <typename Handler, typename... Profiles>
    constexpr bool UniqueAcceptor_impl() {
        return (... + Acceptable<Handler, Profiles>) == 1;
    };

    template <typename... Handlers, typename... Profiles>
    constexpr bool UniqueAcceptor(type_list<Handlers...>, type_list<Profiles...>) {
        return (... && UniqueAcceptor_impl<Handlers, Profiles...>());
    };
}

template <typename... Handlers, typename... Profiles>
constexpr auto pack(
    type_list<Handlers...>,
    Profiles&&... profs
) {
    static_assert(
        detail::UniqueAcceptor(type_list<Handlers...>{}, type_list<Profiles...>{}),
        "Profiles must have exactly one accepting Handler"
    );

    return std::tuple{
        [&]<typename H>(type_list<H>) {
            auto result = std::tuple_cat(
                [&]<typename P>(P&& prof){
                    if constexpr (detail::Acceptable<H, P>)
                        return std::forward_as_tuple(prof);
                    else return std::tuple<>{};
                }(profs)...
            );
            return tpm<H>(std::apply(H::make_container, result));
        }(type_list<Handlers>{})...
    };
}

template <typename... Methods>
void parse(std::tuple<Methods...>& m, std::span<const char*> argv) {
    auto it = argv.begin();
    const auto end = argv.end(); 
    auto mark = it;
    while(it < end) {
        std::apply(
            [&]<typename... Mth>(Mth&&... method) {
                (... || method(&it));
            },
            m
        );
        if(it == mark) 
            throw std::runtime_error("Invalid input or Internal Bug because of stationary iterator");
        mark = it;
    }
}

template <typename... MarkedCont, typename... Args>
requires (... && util::SpecializationOf<MarkedCont, type_mark>) &&
         (... && util::SpecializationOf<Args, type_mark>)
auto create_method(
    const std::tuple<MarkedCont...>& conts,
    Args&&... args
) {
    return std::apply(
        [&]<typename... MCs>(MCs&&... marked_container) {
            return std::make_tuple(
                [&]<typename MC>(MC&& mc) {
                    auto match = std::tuple_cat([&]<typename Arg>(Arg&& arg) {
                        if constexpr (
                            std::same_as<typename MC::mark_type, typename Arg::mark_type>
                        ) return std::forward_as_tuple(arg);
                        else return std::tuple<>{};
                    }(args)...);

                    static_assert(
                        std::tuple_size_v<decltype(match)> <= 1,
                        "No more than one argument is allowed to be forwarded to the same Handler"
                    );

                    return std::apply(MC::mark_type::make_method, std::tuple_cat(std::forward_as_tuple(mc), match));
                }(marked_container)...
            );
        },
        conts
    );
    
}
}