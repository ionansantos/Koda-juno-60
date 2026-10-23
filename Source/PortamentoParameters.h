#pragma once
#include "Parameters.h"
namespace talclone {
enum PortamentoParameter : std::size_t { portamentoMode,portamentoAmount,portamentoTime,portamentoPoly,portamentoParameterCount };
inline constexpr std::array<ParameterSpec,portamentoParameterCount> portamentoSpecs {{
 {"portamentoMode","Portamento Mode",0,2,1,0},
 {"portamentoAmount","Portamento Amount",0,1,0,0},
 {"portamentoTime","Portamento Time",0,1,1,0},
 {"portamentoPoly","Portamento Poly",0,1,1,0}
}};
}
