// Author: watsonryan
// Purpose: Minimal downstream compile/link smoke for installed hwm14 package.

#include "hwm14/hwm14.hpp"

int main() {
  hwm14::Inputs in{};
  in.altitude_km = 250.0;
  return in.altitude_km > 0.0 ? 0 : 1;
}
