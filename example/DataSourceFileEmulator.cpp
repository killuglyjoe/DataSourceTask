#include "DataSourceFileEmulator.h"

#include <atomic>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

static std::atomic<bool> is_write_active {true};

using namespace DATA_SOURCE_TASK;

DataSourceFileEmulator::DataSourceFileEmulator(
    const std::string & file_path,
    const DATA_SOURCE_TASK::SOURCE_TYPE & s_type,
    const DATA_SOURCE_TASK::PAYLOAD_TYPE & p_type,
    const int & frame_size):
    m_file_path {file_path},
    m_elapsed {0.}
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

    // Потік який читає данні
    m_write_thread = std::thread(&DataSourceFileEmulator::writeData, this);
}

void DataSourceFileEmulator::generateRandom()
{
    static float val = 1.2;
    // ++val;
    // Згенеруємо випадкові числа
    for (uint32_t i = 0; i < m_buffer->totalElements(); ++i)
    {
        // float val = m_dist(m_mt);
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

Timer overall_timer; // між записом в файл
Timer diff_timer;    // для вирівнювання sleep До 200 Гц

std::mutex write_lock;
void DataSourceFileEmulator::writeData()
{
    overall_timer.reset();
    diff_timer.reset();

    is_write_active = true;

    std::ofstream source_file(m_file_path, std::ios::out | std::ios::binary);

    static int frm_counter = 0;

    while (is_write_active)
    {
        // Write data to the file
        {
            m_elapsed = overall_timer.elapsed();
            overall_timer.reset();

            diff_timer.reset();

            // generateRandom();

            // Будемо перезаписувати файл блоком даних.
            // Імітується файловий пристрій.
            if (source_file.is_open())
            {
                source_file.seekp(0);

                std::lock_guard<std::mutex> lock(write_lock);
                if (source_file.write(m_buffer->data(), m_buffer->size()))
                {
                    // source_file.flush();
                }
                else
                {
                    if (source_file.fail())
                    {
                        std::ios_base::iostate state = source_file.rdstate();

                        if (state & std::ios_base::eofbit)
                        {
                            std::cout << "DataSourceFileEmulator End of file reached." << std::endl;
                        }
                        if (state & std::ios_base::failbit)
                        {
                            std::cout << "DataSourceFileEmulator Non-fatal I/O error occurred." << std::endl;
                        }
                        if (state & std::ios_base::badbit)
                        {
                            std::cout << "DataSourceFileEmulator Fatal I/O error occurred." << std::endl;
                        }
                    }
                }
            }
            else
            {
                is_write_active = false;
                throw std::runtime_error("DataSourceFileEmulatorFailed to open file for writing." + m_file_path);
            }

            ++frm_counter;
            if (frm_counter >= UINT16_MAX)
                frm_counter = 0;

            m_buffer->setFrameCounter(++frm_counter);
        }

        while (diff_timer.elapsed() < DATA_SOURCE_TASK::FRAME_RATE)
        { // 200 Hz
        }
    }
}

DataSourceFileEmulator::~DataSourceFileEmulator()
{
    std::cout << "~DataSourceFileEmulator()" << std::endl;

    if (m_source_file.is_open())
        m_source_file.close();

    if (m_write_thread.joinable())
        m_write_thread.join();
}
