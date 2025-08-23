#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

LOG_MODULE_REGISTER(statistics, LOG_LEVEL_DBG);

#define MAX_SAMPLES 100

struct ping_statistics {
    uint32_t samples[MAX_SAMPLES];
    uint32_t count;
    uint32_t total_count;
    uint32_t min_rtt;
    uint32_t max_rtt;
    uint64_t sum_rtt;
    uint32_t lost_count;
};

static struct ping_statistics stats;

void statistics_init(void)
{
    memset(&stats, 0, sizeof(stats));
    stats.min_rtt = UINT32_MAX;
    LOG_DBG("Statistics module initialized");
}

void statistics_add_sample(uint32_t rtt_us)
{
    if (stats.count < MAX_SAMPLES) {
        stats.samples[stats.count] = rtt_us;
        stats.count++;
    } else {
        /* Circular buffer - overwrite oldest */
        uint32_t idx = stats.total_count % MAX_SAMPLES;
        stats.samples[idx] = rtt_us;
    }

    stats.total_count++;
    stats.sum_rtt += rtt_us;

    if (rtt_us < stats.min_rtt) {
        stats.min_rtt = rtt_us;
    }
    if (rtt_us > stats.max_rtt) {
        stats.max_rtt = rtt_us;
    }

    LOG_DBG("Sample added: %u us", rtt_us);
}

void statistics_add_lost(void)
{
    stats.lost_count++;
    stats.total_count++;
    LOG_DBG("Lost packet recorded");
}

uint32_t statistics_get_average(void)
{
    if (stats.count == 0) {
        return 0;
    }
    
    uint32_t successful = stats.total_count - stats.lost_count;
    if (successful == 0) {
        return 0;
    }
    
    return (uint32_t)(stats.sum_rtt / successful);
}

uint32_t statistics_get_min(void)
{
    return (stats.min_rtt == UINT32_MAX) ? 0 : stats.min_rtt;
}

uint32_t statistics_get_max(void)
{
    return stats.max_rtt;
}

float statistics_get_loss_rate(void)
{
    if (stats.total_count == 0) {
        return 0.0f;
    }
    
    return ((float)stats.lost_count / (float)stats.total_count) * 100.0f;
}

uint32_t statistics_get_jitter(void)
{
    if (stats.count < 2) {
        return 0;
    }
    
    uint32_t avg = statistics_get_average();
    uint64_t variance = 0;
    
    for (uint32_t i = 0; i < stats.count; i++) {
        int32_t diff = (int32_t)stats.samples[i] - (int32_t)avg;
        variance += (diff * diff);
    }
    
    variance /= stats.count;
    
    /* Simple integer square root approximation */
    uint32_t result = 0;
    uint32_t bit = 1U << 30;
    
    while (bit > variance) {
        bit >>= 2;
    }
    
    while (bit != 0) {
        if (variance >= result + bit) {
            variance -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    
    return result;
}

void statistics_reset(void)
{
    statistics_init();
    LOG_INF("Statistics reset");
}

void statistics_print_summary(void)
{
    LOG_INF("=== Ping Statistics ===");
    LOG_INF("Packets: Sent = %u, Received = %u, Lost = %u (%.1f%% loss)",
            stats.total_count,
            stats.total_count - stats.lost_count,
            stats.lost_count,
            statistics_get_loss_rate());
    
    if (stats.count > 0) {
        LOG_INF("RTT: Min = %u us, Max = %u us, Avg = %u us, Jitter = %u us",
                statistics_get_min(),
                statistics_get_max(),
                statistics_get_average(),
                statistics_get_jitter());
    }
}
