#include "DataSourceEmulator.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

static std::atomic<bool> is_write_active {true};

static constexpr int FRAME_RATE {1000 / 200}; // 200 Hz

namespace DATA_SOURCE_TASK
{

DataSourceFileEmulator::DataSourceFileEmulator(
    const DATA_SOURCE_TASK::SOURCE_TYPE & s_type,
    const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type,
    const int & num_elements):
    DataSource(s_type)
{
    try
    {
        // виділимо данні
        switch (p_type)
        {
        case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
            m_byte_size = num_elements * sizeof(std::uint8_t);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::uint8_t>>(num_elements);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
            m_byte_size = num_elements * sizeof(std::int16_t);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::int16_t>>(num_elements);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
            m_byte_size = num_elements * sizeof(std::int32_t);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<std::int32_t>>(num_elements);
            break;
        case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
            m_byte_size = num_elements * sizeof(float);
            m_buffer    = std::make_shared<DATA_SOURCE_TASK::DataSourceBuffer<float>>(num_elements);
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
        m_buffer->setPayloadSize(num_elements);
    }
    else
    {
        std::cout << "DataSourceFileEmulator: no frame" << std::endl;
    }

    // Потік який оновлює данні
    m_write_thread = std::thread(&DataSourceFileEmulator::updateData, this);
}

DataSourceFileEmulator::~DataSourceFileEmulator() { std::cout << "~DataSourceFileEmulator()" << std::endl; }

Timer overall_timer; // між записом в файл
Timer diff_timer;    // для вирівнювання sleep До 200 Гц

std::mutex update_lock;
void DataSourceFileEmulator::updateData()
{
    overall_timer.reset();
    diff_timer.reset();

    is_write_active = true;

    // Create a random number engine using the Mersenne Twister algorithm
    std::random_device rd;
    std::mt19937 mt(rd());

    std::uniform_real_distribution<float> dist;
    // Create a uniform distribution for generating float numbers in the range
    switch (m_buffer->frame()->payload_type)
    {
    case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
        dist = std::uniform_real_distribution<float>(0, 254);
    case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
        dist = std::uniform_real_distribution<float>(-1450.13f, 1450.13f);
        break;
    case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
        dist = std::uniform_real_distribution<float>(-1450.13f, 1450.13f);
        break;
    case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
        dist = std::uniform_real_distribution<float>(-100.0f, 150.13f);
        break;
    default:
        break;
    }

    static int frm_counter = 0;
    char * payload         = m_buffer->payload();

    while (is_write_active)
    {
        std::lock_guard<std::mutex> lock(update_lock);
        diff_timer.reset();
        // Згенеруємо випадкові числа
        for (int i = 0; i < m_buffer->payloadSize(); ++i)
        {
            float val = dist(mt);
            switch (m_buffer->frame()->payload_type)
            {
            case PAYLOAD_TYPE::PAYLOAD_TYPE_8_BIT_UINT:
                memcpy(payload + i * sizeof(std::uint8_t), &val, sizeof(std::uint8_t));
                break;
            case PAYLOAD_TYPE::PAYLOAD_TYPE_16_BIT_INT:
                memcpy(payload + i * sizeof(std::int16_t), &val, sizeof(std::int16_t));
                break;
            case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_INT:
                memcpy(payload + i * sizeof(std::int32_t), &val, sizeof(std::int32_t));
                break;
            case PAYLOAD_TYPE::PAYLOAD_TYPE_32_BIT_IEEE_FLOAT:
                memcpy(payload + i * sizeof(float), &val, sizeof(float));
                break;
            default:
                break;
            }
        }

        m_buffer->setFrameCounter(++frm_counter);

        double dif_ms = diff_timer.elapsed_ms();
        if (dif_ms < FRAME_RATE)
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_RATE - static_cast<int>(dif_ms))); // 200 Hz

        m_elapsed = overall_timer.elapsed();
        overall_timer.reset();
    }
}

std::mutex read_lock;
int DataSourceFileEmulator::read(char * data, int size)
{
    std::lock_guard<std::mutex> lock(read_lock);
    std::copy(m_buffer->data(), m_buffer->data() + size, data);

    return size;
}

} // namespace DATA_SOURCE_TASK
