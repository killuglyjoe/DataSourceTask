#include "DataSourceEmulator.h"

#include <iostream>
#include <mutex>

namespace DATA_SOURCE_TASK
{

static constexpr bool is_used_random {false};

float randMinToMax(const float & min, const float & max)
{
    return min + (rand() / (RAND_MAX / (max - min)));
}

DataSourceFileEmulator::DataSourceFileEmulator(const DATA_SOURCE_TASK::SOURCE_TYPE & s_type,
                                               const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type,
                                               const int & frame_size):
    DataSource(s_type)
{
    try
    {
        // виділимо данні
        m_buffer = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::uint8_t>>(frame_size);
    }
    catch (const std::exception & e)
    {
        std::runtime_error(e.what()); // треба використовувати власні обгортки над стандартнимим виключеннями
    }

    if (m_buffer->frame())
    {
        m_buffer->setHeader(0xf113); // file
        m_buffer->setFrameCounter(0);
        m_buffer->setSourceID(s_type);
        m_buffer->setPayloadType(p_type);
    }
    else
    {
        std::cout << "DataSourceFileEmulator: no frame" << std::endl;
    }

    srand((unsigned int) time(NULL));
    generateRandom();
}

DataSourceFileEmulator::~DataSourceFileEmulator() {}

void DataSourceFileEmulator::generateRandom()
{
    static float val = 1.f;

    if (!is_used_random)
        val += 1.f;

    switch (m_buffer->frame()->payload_type)
    {
    case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
    {
        std::uint8_t * payload = reinterpret_cast<std::uint8_t *>(m_buffer->payload());

        static const uint32_t total_elements = m_buffer->payloadSize() / sizeof(std::uint8_t);

        for (uint32_t i = 0; i < total_elements; ++i)
        {
            if (is_used_random)
                val = randMinToMax(0, UINT8_MAX);

            payload[i] = static_cast<std::uint8_t>(val);
        }
    }
    break;

    case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
    {
        std::int16_t * payload = reinterpret_cast<std::int16_t *>(m_buffer->payload());

        static const uint32_t total_elements = m_buffer->payloadSize() / sizeof(std::int16_t);

        for (uint32_t i = 0; i < total_elements; ++i)
        {
            if (is_used_random)
                val = randMinToMax(INT16_MIN, INT16_MAX);

            payload[i] = static_cast<std::int16_t>(val);
        }
    }
    break;

    case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
    {
        std::int32_t * payload = reinterpret_cast<std::int32_t *>(m_buffer->payload());

        static const uint32_t total_elements = m_buffer->payloadSize() / sizeof(std::int32_t);

        for (uint32_t i = 0; i < total_elements; ++i)
        {
            if (is_used_random)
                val = randMinToMax(INT32_MIN, INT32_MAX);

            payload[i] = static_cast<std::int32_t>(val);
        }
    }
    break;

    case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
    {
        float * payload = reinterpret_cast<float *>(m_buffer->payload());
        static const uint32_t total_elements = m_buffer->payloadSize() / sizeof(float);

        for (uint32_t i = 0; i < total_elements; ++i)
        {
            if (is_used_random)
                val = randMinToMax(INT32_MIN, INT32_MAX);

            payload[i] = val;
        }
    }
    break;

    default:
        break;
    }
}

void DataSourceFileEmulator::updateBufs()
{
    static uint16_t frm_counter = 0;

    generateRandom();

    ++frm_counter;
    if (frm_counter >= UINT16_MAX)
        frm_counter = 0;

    m_buffer->setFrameCounter(frm_counter);
}

int DataSourceFileEmulator::read(char * data, int size)
{
    int ret_size = size;

    static Timer overall_timer; // між оновленням даних
    static Timer diff_timer;    // для вирівнювання sleep До 200 Гц

    static double elapsed = 0.;

    std::lock_guard<std::mutex> lock(m_read_lock);

    overall_timer.reset();
    diff_timer.reset();

    elapsed = 0.;

    updateBufs();

    static int b = 0;
    if (b < 10)
    {
        // - результат DataSource::read() непередбачуваний, близький до реальної ситуації, може варіюватись у межах 0..size;
        ret_size = size / 2;
        ++b;
    }

    std::copy(m_buffer->data(), m_buffer->data() + ret_size, data);

    elapsed = overall_timer.elapsed();

    m_elapsed = elapsed;

    return ret_size;
}

} // namespace DATA_SOURCE_TASK
