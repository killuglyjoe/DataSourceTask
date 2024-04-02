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
class DataSourceController final : public DataSourceFrameProcessor
{
public:
    /// \brief Конструктор класу. Тут виділомо память під клас DataSource
    /// і відповідно пам'ять для кадру
    /// \param source_path - безпосередньо походження джерела (шлях до файлу, мережева адреса тощо). Може треба
    /// параметризувати цей параметр \param source_type - тип джереала \param p_type - тип корисних даних \param
    /// frame_size - к-сть елементів в payload
    DataSourceController(const std::shared_ptr<DataSource> & data_source, const std::uint32_t & frame_size);

    virtual ~DataSourceController();

    /// \brief Значення magic_word
    /// \return
    inline int header() { return m_buffer->header(); }

    /// \brief Поточний лічильник кадрів
    /// \return
    inline int framesTotal() { return m_buffer->frameCounter(); }

    /// \brief Час читання з джерела. Повинен бути менше FRAME_RATE.
    /// \return
    inline double elapsed() { return m_elapsed; }

    /// \brief Час запису нового фрейма
    /// \return
    inline double writeFramelapsed() { return m_data_source->readElapsed(); }

protected:
    /// \brief Потокова функція читання даних з джерела циклом з частотою FRAME_RATE
    void readData();

private:
    std::atomic<int> m_elapsed; // час читання з джерела, мс

    std::atomic<bool> m_is_read_active;

    std::thread m_read_thread;

    std::shared_ptr<DATA_SOURCE_TASK::DataSource> m_data_source;

    std::shared_ptr<DataSourceBufferInterface> m_buffer; // масиви для зберішання вх. даних

    std::mutex m_mutex;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCECONTROLLER_H
