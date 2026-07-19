#include <concepts>
#include <type_traits>
#include <cstddef>

namespace miz {
namespace util {

template <typename T, typename... Range>
static constexpr std::size_t count_types = (std::same_as<T, Range> + ...);

template <typename T>
concept is_complete_type = requires { sizeof(T); };

template <typename T, typename... Range>
concept is_within = count_types<T, Range...> > 0;

template <typename T, typename... Range>
concept contains_duplicate = count_types<T, Range...> > 1;

template <typename... Ts>
concept contains_any_duplicate = (contains_duplicate<Ts, Ts...> || ...);

template <typename T, template <typename...> class Templ>
struct specialization_of : std::false_type {};

template <template <typename...> class Templ, typename... Args>
struct specialization_of<Templ<Args...>, Templ> : std::true_type {};

template <typename T, template <typename...> class Templ>
concept SpecializationOf = specialization_of<T, Templ>::value;

}
}