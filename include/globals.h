#ifndef GLOBALS_H
#define GLOBALS_H

#include <cstdint>
#include <ctime>

namespace DATA_SOURCE_TASK
{
class Timer
{
public:
    Timer() { clock_gettime(CLOCK_REALTIME, &beg_); }

    /// \brief Пройдений час в секундах
    /// \return
    double elapsed() {
        clock_gettime(CLOCK_REALTIME, &end_);
        return end_.tv_sec - beg_.tv_sec +
               (end_.tv_nsec - beg_.tv_nsec) / 1000000000.;
    }
    /// \brief Пройдений час в мілісекундах
    /// \return
    double elapsed_ms() {
        clock_gettime(CLOCK_REALTIME, &end_);
        return end_.tv_sec - beg_.tv_sec +
               (end_.tv_nsec - beg_.tv_nsec) / 1000000.;
    }

    void reset() { clock_gettime(CLOCK_REALTIME, &beg_); }

private:
    timespec beg_, end_;
};
// максимальний розмір запитуваних даних size обмежується ресурсами обчислювальної системи
static constexpr std::uint32_t MAX_DATA_SIZE {20 * 1024};

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
    std::int32_t magic_word;    // ідентифікатор початку кадру (magic_word) 4 байти;
    std::int16_t frame_counter; // циклічний лічильник кадрів (frame_counter) 2 байти;
    SOURCE_TYPE source_id;      // ідентифікатор походження (source_id) 1 байт;
    PAYLOAD_TYPE payload_type;  // тип даних (payload_type) 1 байт;
    std::int32_t payload_size;  // розмір блоку даних корисного навантаження (payload_size) 4 байти;
    void * payload;             // дані payload являють собою оцифовані відліки сигналу.
                                // розмір блоку даних корисного навантаження може бути різний, зазвичай кратний 4 байтам
};
#pragma pack()

} // namespace DATA_SOURCE_TASK

#endif // GLOBALS_H
