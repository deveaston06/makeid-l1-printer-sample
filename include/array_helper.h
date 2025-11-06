#include <vector>

template <typename T>
void pushArrayToAnother(std::vector<T> source, std::vector<T> dest) {
  int i = source.size();
  while (i > 0) {
    i--;
    dest.push_back(source[i]);
  }
}
