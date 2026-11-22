#include "filtering/filter_design.hpp"

namespace filtering {

// Explicit template instantiations of design utilities
template std::vector<float> FilterDesign::create_moving_average<float>(std::size_t);
template std::vector<double> FilterDesign::create_moving_average<double>(std::size_t);

template std::vector<float> FilterDesign::create_lowpass_fir<float>(std::size_t, float);
template std::vector<double> FilterDesign::create_lowpass_fir<double>(std::size_t, double);

template std::vector<float> FilterDesign::create_highpass_fir<float>(std::size_t, float);
template std::vector<double> FilterDesign::create_highpass_fir<double>(std::size_t, double);

} // namespace filtering
