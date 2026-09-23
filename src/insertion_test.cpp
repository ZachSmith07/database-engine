#include "insertion_test.h"
#include <iostream>
#include <chrono>

#include "schema.h"
#include "tuple.h"
#include "value.h"
#include "heap_page.h"
#include "page_id.h"
#include "page.h"
#include "heap_manager.h"

void test_insert(PageManager &pm, WalManager &wm)
{
    // SCHEMA
    TableSchemaBuilder b(1, "cars");

    auto col_make = b.add_column("make", Type::Text(), false);
    auto col_model = b.add_column("model", Type::Text(), false);
    auto col_year = b.add_column("year", Type::Int32(), false);
    auto col_vin = b.add_column("vin", Type::Varchar(17), false);

    TableSchema cars = b.move_build();

    // ROW
    Row row;
    row[col_make] = Value::Text("Toyota");
    row[col_model] = Value::Text("Corolla");
    row[col_year] = Value::Int32(2020);
    row[col_vin] = Value::Varchar("JTDBR32E720045678");

    constexpr int N = 500000; // number of rows to insert
    uint32_t xmin = 1;

    HeapManager heap_manager{pm, wm};

    size_t inserted = 0;
    size_t page_count = 1;

    // TIMING
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < N; ++i)
    {
        row[col_year] = Value::Int32(i);
        heap_manager.insert_row(cars, row, xmin, 1); // everything here is calculated (like row to the actual bytes), then saved and wal + writing to SSD
        inserted++;
    }

    wm.flush();

    auto end = std::chrono::high_resolution_clock::now();

    auto total_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // RESULTS
    std::cout << "Inserted tuples: " << inserted << "\n";
    std::cout << "Pages used:      " << page_count << "\n";
    std::cout << "Tuples/page:    "
              << (inserted / page_count) << "\n";
    std::cout << "Total time:     " << total_ns << " ns\n";
    std::cout << "Avg per insert: " << (total_ns / inserted) << " ns\n";
    std::cout << wm.current_lsn() << std::endl;
}