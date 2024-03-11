#ifndef GLOBALS_H
#define GLOBALS_H

#include <atomic>
#include <cstdint>
#ifdef WIN32
#include <profileapi.h>
#include <winnt.h>
#else
#include <chrono>
#endif

namespace DATA_SOURCE_TASK
{
#ifdef WIN32
class Timer
{
public:
    Timer() { reset(); }

    void reset()
    {
        // Get the frequency of the performance counter
        QueryPerformanceFrequency(&frequency);

        // Record the start time
        QueryPerformanceCounter(&start);
    }
    double elapsed()
    {
        LARGE_INTEGER end;
        QueryPerformanceCounter(&end);

        // Calculate the elapsed time in milliseconds
        return static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;
    }

private:
    LARGE_INTEGER start;
    LARGE_INTEGER frequency;
};
#else
class Timer
{
public:
    Timer() { reset(); }

    bool isValid() const { return m_is_valid; }
    void reset()
    {
        m_start    = std::chrono::system_clock::now();
        m_is_valid = true;
    }

    /// \brief Пройдений час в мілісекундах
    /// \return
    std::int64_t elapsed() const
    {
        if (isValid())
        {
            static std::chrono::time_point<std::chrono::system_clock> end;
            end = std::chrono::system_clock::now();

            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::high_resolution_clock::now() - m_start)
                .count();
        }
        return std::numeric_limits<int64_t>::max();
    }

private:
    std::atomic<bool> m_is_valid;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};
#endif

static constexpr double FRAME_RATE {1000. / 200.}; // 200 Hz = 5ms

// К-сть буферів під читання/обробку
static constexpr std::size_t MAX_PROCESSING_BUF_NUM {10};

// тип даних payload_type може бути:
enum class PAYLOAD_TYPE : char
{
    PAYLOAD_TYPE_8_BIT_UINT = 0u,   //  8 bit unsigned int;
    PAYLOAD_TYPE_16_BIT_INT,        //  16 bit signed int;
    PAYLOAD_TYPE_32_BIT_INT,        //  32 bit signed int;
    PAYLOAD_TYPE_32_BIT_IEEE_FLOAT, //  32 bit IEEE 754 float;
    PAYLOAD_TYPE_UNSUPPORTED = 127u
};

// припускаємо шо можуть бути наступні типи джерел
enum class SOURCE_TYPE : char
{
    SOURCE_TYPE_FILE = 0u,
    SOURCE_TYPE_EMULATOR,
    SOURCE_TYPE_UNDEFINED
};

enum class DATA_SOURCE_ERROR : int
{
    READ_SOURCE_ERROR = -1,
};

#pragma pack(1)
// дані мають кадрову структуру (приблизно):
struct frame
{
    std::uint32_t magic_word;       // ідентифікатор початку кадру (magic_word) 4 байти;
    std::uint16_t frame_counter;    // циклічний лічильник кадрів (frame_counter) 2 байти;
    SOURCE_TYPE source_id;          // ідентифікатор походження (source_id) 1 байт;
    PAYLOAD_TYPE payload_type;      // тип даних (payload_type) 1 байт;
    std::uint32_t payload_size;     // розмір блоку даних корисного навантаження (payload_size) 4 байти;
    void * payload;                 // дані payload являють собою оцифовані відліки сигналу.
                                    // розмір блоку даних корисного навантаження може бути різний, зазвичай кратний 4 байтам
};
#pragma pack()

static constexpr std::uint32_t FRAME_HEADER_SIZE {sizeof(struct frame) - sizeof(void *)};

} // namespace DATA_SOURCE_TASK

#endif // GLOBALS_H
