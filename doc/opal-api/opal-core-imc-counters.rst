The interface can be extended to include more modes and operations
available in core IMC.

Parameters
----------
``uint64_t operation``
  3 operations are supported for now (include/opal-api.h) :
  - OPAL_CORE_IMC_DISABLE -- disable or stop the counters
  - OPAL_CORE_IMC_ENABLE -- enable/resume the counters
  - OPAL_CORE_IMC_INIT -- Initialize and enable the counters.

``uint64_t addr``
  For OPAL_CORE_IMC_INIT, this parameter must have a non-zero value.
  This value must be a per-core physical address.
  For other operations, this value is undefined and must be zero.

``uint64_t value_1``
  - This parameter is undefined for now and must be zero.

``uint64_t value_2``
  - This parameter is undefined for now and must be zero.

Returns
-------
OPAL_PARAMETER
  In any one/some/all of the following cases :
  - Unsupported ``operation``
  - Wrong ``addr`` for the given ``operation``
  - Non-zero ``value_1`` or ``value_2``
OPAL_HARDWARE
   If xscom_write fails.
OPAL_SUCCESS
   On successful execution of the ``operation``.
