#ifndef DATASOURCEFRAMEPROCESSOR_H
#define DATASOURCEFRAMEPROCESSOR_H

#include "DataSourceBuffer.h"
#include "DataSourceFrameRecorder.h"

#include <memory>
#include <mutex>
#include <atomic>

namespace DATA_SOURCE_TASK
{

enum class DATA_SOURCE_HW_CONV_TYPE : int
{
    DATA_SOURCE_HW_CONV_TYPE_CPU = 0,
    DATA_SOURCE_HW_CONV_TYPE_GPU
};

// Буферізація масивів вхідних даних
static constexpr int BUFERIZATION_NUM {2};

/// \brief Клас для валідації отриманого кадру з джерела даних.
/// Робить перевірку і складання кадрів.
/// Заповнює буфери масивів даних, розмірністю MxN. К-сть буферів BUFERIZATION_NUM,
/// а вних к-сть кадрів MAX_PROCESSING_BUF_NUM.
class DataSourceFrameProcessor
{
public:
    /// \brief Клас для роботи з отриманимим кадрами.
    DataSourceFrameProcessor(const int & frame_size);
    virtual ~DataSourceFrameProcessor();

    /// \brief Перевірка бракованих кадрів.
    /// Конвертація в float.
    /// \param frm
    /// \param updated_size
    bool validateFrame(std::shared_ptr<DataSourceBufferInterface> & buffer);
    /// \brief Розмір кадру
    /// \return
    inline int frameSize() const { return m_frame_size; }
    /// \brief К-сть втрачених пакетів, рахуються по лячильнику в заголовку кадру
    /// \return
    inline int getPacketsLoss() const { return m_packets_loss; }
    /// \brief Браковані кадри.
    /// Рахуються по результату методу read, якщо розмір прочитаних даних менше бажаного.
    /// \return
    inline int getBadFrames() const { return m_bad_frames; }
    /// \brief Функція записує вх. кадр в масив буферів кадрів.
    /// \param buffer
    /// \param updated_size
    void putNewFrame(std::shared_ptr<DataSourceBufferInterface> & buffer, const int & updated_size);
    /// \brief Пройдений час на обробки вх. даних в потоці.
    /// \return мілісекунди
    inline double elapsed() { return m_elapsed; }
    /// \brief Час запису оброблених даних в файл.
    /// \return мілісекунди
    inline double saveFrameElapsed() { return m_data_source_recorder->elapsed(); }

protected:
    /// \brief Потокова функція обробки вхідних буферів
    void frameProcess();

private:
    /// \brief перетворення масиву цілих чисел в float[]
    /// \param payload - масив цілих чисел
    void convertToFLoat(char * payload);

private:
    int m_frame_size   = 0; // відомий розмір кадру
    int m_packets_loss = 0; // втрати пакетів на основі лфчильника кадрів
    int m_bad_frames = 0; // поганий пакет на основі повернутого розміру кадру

    double m_elapsed = 0; // час обробки вх. даних, мс

    std::mutex m_process_mutex;

    std::atomic<bool> m_can_validate;
    std::atomic<int> m_req_size;

    // --------------   Дані з джерела   --------------------
    std::atomic<int> m_src_ready_buffer; // 0..MAX_PROCESSING_BUF_NUM-1
    std::atomic<int> m_active_buffer;    // 0 або 1

    // Два буфери - один наповнюємо з іншим працюємо. Потім навпаки
    std::shared_ptr<DataSourceBufferInterface> m_source_buffer[BUFERIZATION_NUM]
                                                              [MAX_PROCESSING_BUF_NUM]; // дані для swap з джерела

    // --------------   Оброблені дані (float)   --------------------
    std::atomic<int> m_flt_ready_buffer;                            // 0..MAX_PROCESSING_BUF_NUM-1
    std::vector<std::shared_ptr<DataSourceBuffer<float>>> m_buffer; // дані будуть перетворені в float

    std::unique_ptr<DataSourceFrameRecorder> m_data_source_recorder; // клас для запису оброблених даних в файл.
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCEFRAMEPROCESSOR_H
