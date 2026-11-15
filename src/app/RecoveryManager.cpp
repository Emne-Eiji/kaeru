// Kaeru — recovery manager (spec §6.6, WP-05 wiring)
#include "RecoveryManager.h"

namespace kaeru {

// WP-02: only the neutral empty report exists. Full interrupted-session
// detection and conservative recovery are implemented in WP-05 against the
// storage layer (recovery_checkpoints; checkpoints never duplicate report
// entries because totals read only closed session_segments).

} // namespace kaeru
