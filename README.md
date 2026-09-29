# Database Engine

A storage engine written from scratch in C++, being built towards a relational database that supports SQL. I'm building it to learn how real databases work, following the design of systems like PostgreSQL.

## Status

Work in progress.

- [x] Heap table storage with 8 KB slotted pages
- [x] B+ tree indexes
- [x] Tuple headers with `xmin` / `xmax` fields for future MVCC
- [x] Write-ahead log with per-record checksums
- [ ] Crash recovery (replay from checkpoint) <!-- tick if done -->
- [ ] SQL parsing and query execution
- [ ] Transactions and visibility rules
- [ ] Vacuum and background page writer

## Performance

Inserting 500,000 rows of 63 bytes each:

| Metric | Result |
|---|---|
| Total time | 214.9 ms |
| Average per insert | 429 ns |
| Throughput | 2.3 million rows/s |

<!-- State the hardware and compiler flags, and whether the WAL is flushed to disk per insert, per batch, or only at the end. -->

## Design overview

**Heap table, not a clustered index.** Rows are appended to pages as they are inserted and addressed by (page number, slot number), which fits in about 8 bytes. Compared with a clustered B+ tree, inserts are cheaper and secondary indexes don't have to store variable-length primary keys or do a second tree lookup.

**Write-ahead log.** Changes are written to a sequential log and flushed on commit, rather than writing every heap and index page to disk on every request. Pages stay in memory and a background writer flushes them later. After a crash, the log is replayed from the last checkpoint, using each page's log sequence number and checksums to work out what was already written.

## History

This is a rewrite. My first version supported indexes (b-tree instead of b+) and searching but managed only about 1 ms per row insert. After researching how production databases are built, I started over.
