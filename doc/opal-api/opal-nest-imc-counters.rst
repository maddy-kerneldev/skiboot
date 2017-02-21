OPAL_NEST_IMC_COUNTERS_CONTROL
==============================

OPAL call interface for per-chip nest instrumentation support.
Currently, the interface supports starting and stopping the In Memory
Collection Microcode running in the OCC complex for Nest
Instrumentation from Host OS.

The interface can be extended to include more modes and operations to use
the multiple debug modes for nest IMC instrumentation.

Parameters
----------
``uint64_t mode``
  Currently, only production mode is supported.
  The "NEST_IMC_PRODUCTION_MODE" macro defined in "include/imc.h"
  is used for this.

``uint64_t operation``
  For "NEST_IMC_PRODUCTION_MODE" mode, this parameter is used
  to start and stop the IMC microcode. (include/opal-api.h)

  - OPAL_NEST_IMC_STOP -- Stop
  - OPAL_NEST_IMC_START -- Start

  For other modes, this parameter is undefined for now.

``uint64_t value_1``
  - For "NEST_IMC_PRODUCTION_MODE" mode, this parameter should be
    zero.
  - For other modes, this parameter is undefined for now.

``uint64_t value_3``
  - For "NEST_IMC_PRODUCTION_MODE" mode, this parameter should be
    zero.
  - For other modes, this parameter is undefined for now.

Returns
-------
OPAL_PARAMETER
  In any one/some/all of the following cases :

  - Unsupported ``mode``
  - Unsupported ``operation``
  - Non-zero ``value_1`` or ``value_2``
OPAL_HARDWARE
   If xscom_write fails.
OPAL_SUCCESS
   On successful execution of the ``operation`` in the given ``mode``.
