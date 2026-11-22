#include "filtering/fir_filter.hpp"

namespace filtering {

// Explicit template instantiations for float and double
template class FirFilter<float>;
template class FirFilter<double>;

} // namespace filtering
