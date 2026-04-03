/* Include files */

#include "ACCSimulationByUsingPID_cgxe.h"
#include "m_s8MrUKUS9PT0PjKwNS7sG.h"

unsigned int cgxe_ACCSimulationByUsingPID_method_dispatcher(SimStruct* S, int_T
  method, void* data)
{
  if (ssGetChecksum0(S) == 421620841 &&
      ssGetChecksum1(S) == 1826143301 &&
      ssGetChecksum2(S) == 335232876 &&
      ssGetChecksum3(S) == 3014372535) {
    method_dispatcher_s8MrUKUS9PT0PjKwNS7sG(S, method, data);
    return 1;
  }

  return 0;
}
