#ifndef MONITOR_H
#define MONITOR_H

// FIX: Changed from (const char *, int) to just (int index)
// to match the implementation in monitor.c
void monitor_process_event(int index);
void monitor_init(void);
void monitor_scan_all(void);

#endif