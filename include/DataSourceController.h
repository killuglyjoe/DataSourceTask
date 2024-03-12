#ifndef DATASOURCECONTROLLER_H
#define DATASOURCECONTROLLER_H

#include "DataSource.h"
#include "DataSourceBuffer.h"
#include "DataSourceFrameProcessor.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace DATA_SOURCE_TASK
{

/// \brief Клас контролер для конкретного типу джерела.
class DataSourceController
{
public:
    /// \brief Конструктор класу. Тут виділомо память під клас DataSource
    /// і відповідно пам'ять для кадру
    /// \param source_path - безпосередньо походження джерела (шлях до файлу, мережева адреса тощо). Може треба
    /// параметризувати цей параметр \param source_type - тип джереала \param p_type - тип корисних даних \param
    /// frame_size - к-сть елементів в payload
    DataSourceController(
        const std::string & source_path,
        const SOURCE_TYPE & source_type,
        const PAYLOAD_TYPE & p_type,
        const std::uint32_t & frame_size);

    virtual ~DataSourceController();

    /// \brief Значення magic_word
    /// \return
    inline int header() { return m_buffer->header(); }
    /// \brief К-сть втрачених пакетів, рахуються по лячильнику в заголовку кадру
    /// \return
    inline int getPacketsLoss() const { return m_data_source_frm_processor->getPacketsLoss(); }
    /// \brief Браковані кадри.
    /// Рахуються по результату методу read, якщо розмір прочитаних даних менше бажаного.
    /// \return
    inline int getBadFrames() const { return m_data_source_frm_processor->getBadFrames(); }
    /// \brief Поточний лічильник кадрів
    /// \return
    inline int framesTotal() { return m_buffer->frameCounter(); }
    /// \brief Час читання з джерела. Повинен бути менше FRAME_RATE.
    /// \return
    inline double elapsed() { return m_elapsed; }
    /// \brief Час запису нового фрейма
    /// \return
    inline double writeFramelapsed() { return m_data_source->elapsed(); }
    /// \brief Час запису оброблених даних в файл.
    /// \return мілісекунди
    inline double saveFramelapsed() { return m_data_source_frm_processor->saveFrameElapsed(); }
    /// \brief Пройдений час на обробки вх. даних в потоці.
    /// \return мілісекунди
    inline double frameValidationElapsed() { return m_data_source_frm_processor->elapsed(); }

protected:
    /// \brief Потокова функція читання даних з джерела циклом з частотою FRAME_RATE
    void readData();

private:
    std::atomic<int> m_elapsed; // час читання з джерела, мс

    std::unique_ptr<DataSource> m_data_source;                              // клас джерела даних
    std::unique_ptr<DataSourceFrameProcessor> m_data_source_frm_processor;  // клас для обробки даних
    std::shared_ptr<DataSourceBufferInterface> m_buffer;                    // масиви для зберішання вх. даних

    std::mutex m_mutex;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCECONTROLLER_H
