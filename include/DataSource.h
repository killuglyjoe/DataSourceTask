#ifndef DATASOURCE_H
#define DATASOURCE_H

#include "globals.h"

namespace DATA_SOURCE_TASK
{

/// \brief Абстрактний клас певного джерела даних
class DataSource
{
public:
    /// \brief Конструктор класу
    /// \param source - тип джерела
    explicit DataSource(const SOURCE_TYPE & source);
    virtual ~DataSource() = default;

    /// \brief Вичитуємо кадр з джерела.
    /// Припускаємо шо є конкретний механізм читання з самого джерела (файл, сокет, послідовні порти та ін).
    /// \param data
    /// \param size
    /// \return
    virtual int read(char * data, int size) = 0;

    inline SOURCE_TYPE sourceType() const { return m_source_type; }

private:
    SOURCE_TYPE m_source_type;
};

} // namespace DATA_SOURCE_TASK

#endif // DATASOURCE_H
