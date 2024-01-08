#include <functional>
#include <iterator>
#include <vector>

template <class T>
int find_index(
    const std::vector<T> &v,
    const std::function<bool(const std::type_identity_t<T> &)> predicate) {
  auto iter = std::find_if(v.begin(), v.end(), predicate);
  size_t index = std::distance(v.begin(), iter);
  if (index == v.size()) {
    return -1;
  }
  return index;
}