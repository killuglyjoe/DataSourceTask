#include "DataSourceEmulator.h"

#include <iostream>
#include <mutex>
#include <random>

namespace DATA_SOURCE_TASK
{

DataSourceFileEmulator::DataSourceFileEmulator(
    const DATA_SOURCE_TASK::SOURCE_TYPE & s_type,
    const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type,
    const int & frame_size):
    DataSource(s_type)
{
    try
    {
        // Create a random number engine using the Mersenne Twister algorithm
        std::random_device rd;
        m_mt = std::mt19937(rd());

        // виділимо данні
        switch (p_type)
        {
        case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
            // Create a uniform distribution for generating float numbers in the range
            m_dist      = std::uniform_real_distribution<float>(0, 254);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::uint8_t>>(frame_size);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
            // Create a uniform distribution for generating float numbers in the range
            m_dist      = std::uniform_real_distribution<float>(-1450.13f, 1450.13f);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::int16_t>>(frame_size);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
            // Create a uniform distribution for generating float numbers in the range
            m_dist      = std::uniform_real_distribution<float>(-1450.13f, 1450.13f);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::int32_t>>(frame_size);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
            // Create a uniform distribution for generating float numbers in the range
            m_dist      = std::uniform_real_distribution<float>(-100.0f, 150.13f);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<float>>(frame_size);
            break;
        default:
            break;
        }
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

    generateRandom();
}

DataSourceFileEmulator::~DataSourceFileEmulator() {}

void DataSourceFileEmulator::generateRandom()
{
    static float val = 1.0;
    // ++val;
    // Згенеруємо випадкові числа
    for (uint32_t i = 0; i < m_buffer->totalElements(); ++i)
    {
        val = m_dist(m_mt);
        switch (m_buffer->frame()->payload_type)
        {
        case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
        {
            std::uint8_t * payload = reinterpret_cast<std::uint8_t *>(m_buffer->payload());
            std::uint8_t u8_val    = static_cast<std::uint8_t>(val);
            payload[i]             = u8_val;
        }
        break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
        {
            std::int16_t * payload = reinterpret_cast<std::int16_t *>(m_buffer->payload());
            std::int16_t s16_val   = static_cast<std::int16_t>(val);
            payload[i]             = s16_val;
        }
        break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
        {
            std::int32_t * payload = reinterpret_cast<std::int32_t *>(m_buffer->payload());
            std::int32_t s32_val   = static_cast<std::int32_t>(val);
            payload[i]             = s32_val;
        }
        break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
        {
            float * payload = reinterpret_cast<float *>(m_buffer->payload());
            float fl_val    = static_cast<float>(val);
            payload[i]      = fl_val;
        }
        break;
        default:
            break;
        }
    }
}

void DataSourceFileEmulator::updateBufs()
{
    static uint16_t frm_counter = 0;

    // generateRandom();

    ++frm_counter;
    if (frm_counter >= UINT16_MAX)
        frm_counter = 0;

    m_buffer->setFrameCounter(frm_counter);
}

int DataSourceFileEmulator::read(char * data, int size)
{
    static Timer overall_timer; // між оновленням даних
    static Timer diff_timer; // для вирівнювання sleep До 200 Гц

    std::lock_guard<std::mutex> lock(m_read_lock);

    overall_timer.reset();
    diff_timer.reset();

    m_elapsed = 0;

    updateBufs();

    // - результат DataSource::read() непередбачуваний, близький до реальної ситуації, може варіюватись у межах 0..size;
    // int ret_size = size; // size / 2;

    std::copy(m_buffer->data(), m_buffer->data() + size, data);

    // while (m_elapsed < FRAME_RATE)
    // {
    //     m_elapsed = overall_timer.elapsed();
    // }

    return size;
}

} // namespace DATA_SOURCE_TASK
