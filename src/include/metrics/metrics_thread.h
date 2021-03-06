#pragma once

#include <chrono>  //NOLINT
#include <thread>  //NOLINT
#include "di/di_help.h"
#include "metrics/metrics_manager.h"

namespace terrier::metrics {

/**
 * Class for spinning off a thread that runs metrics collection at a fixed interval. This should be used in most cases
 * to enable metrics aggregation in the system unless you need fine-grained control over state or profiling.
 */
class MetricsThread {
 public:
  DECLARE_ANNOTATION(METRICS_PERIOD)
  /**
   * @param metrics_period sleep time between metrics invocations
   */
  BOOST_DI_INJECT(MetricsThread, (named = METRICS_PERIOD) std::chrono::milliseconds metrics_period)  // NOLINT
  : run_metrics_(true),
    /// @cond DOXYGEN_IGNORE // TODO(Matt): no idea why this is currently necessary. Doxygen thinks these are functions
    metrics_paused_(false),
    metrics_period_(metrics_period),
    metrics_thread_(std::thread([this] { MetricsThreadLoop(); })) {}
  /// @endcond

  ~MetricsThread() {
    run_metrics_ = false;
    metrics_thread_.join();
    metrics_manager_.ToCSV();
  }

  /**
   * Pause the metrics from running, typically for use in tests when the state needs to be fixed.
   */
  void PauseMetrics() {
    TERRIER_ASSERT(!metrics_paused_, "Metrics should not already be paused.");
    metrics_paused_ = true;
  }

  /**
   * Resume metrics after being paused.
   */
  void ResumeMetrics() {
    TERRIER_ASSERT(metrics_paused_, "Metrics should already be paused.");
    metrics_paused_ = false;
  }

  /**
   * @return the underlying metrics manager object.
   */
  metrics::MetricsManager &GetMetricsManager() { return metrics_manager_; }

 private:
  metrics::MetricsManager metrics_manager_;
  volatile bool run_metrics_;
  volatile bool metrics_paused_;
  std::chrono::milliseconds metrics_period_;
  std::thread metrics_thread_;

  void MetricsThreadLoop() {
    while (run_metrics_) {
      std::this_thread::sleep_for(metrics_period_);
      if (!metrics_paused_) {
        metrics_manager_.Aggregate();
        metrics_manager_.ToCSV();
      }
    }
  }
};

}  // namespace terrier::metrics
