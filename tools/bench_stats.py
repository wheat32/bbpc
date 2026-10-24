#!/usr/bin/env python3
"""tools/bench_stats.py FRAMES.csv: the summary tools/bench.sh prints (the BB_FRAME_LOG columns).

  --ready                 exits 0 once the last 60 frames each drew over 500 draws (the level)
  --from T --seconds S    the window [T, T + S) of the t_s column
  --threads START END     thread CPU time samples (name|ticks|tid lines) taken at T and T + S
  --gpu FILE              nvidia-smi utilization samples (one percentage per line)
"""
import argparse
import csv
import os
import statistics
import sys

LEVEL_DRAWS = 500
READY_FRAMES = 60
SPREAD_MS = (6.0, 7.0, 8.0, 10.0, 12.0, 15.0)
COLUMNS = ('stage_a_cpu_ms', 'present_wait_ms', 'tick_wait_ms', 'draws', 'dispatches', 'submissions',
           'compiles')
TOP_THREADS = 10


def read_frames(path):
    rows = []
    with open(path) as file:
        for row in csv.DictReader(file):
            try:
                rows.append({key: float(value) for key, value in row.items() if value not in (None, '')})
            except ValueError:
                continue
    return [row for row in rows if 'draws' in row and 'frame_ms' in row]


def percentile(sorted_values, fraction):
    return sorted_values[min(len(sorted_values) - 1, int(len(sorted_values) * fraction))]


def summarize_frames(rows, start, seconds):
    window = [row for row in rows if start <= row['t_s'] < start + seconds and row['frame_ms'] > 0]
    if len(window) < 2:
        print('no frames in the window')
        return False
    times = sorted(row['frame_ms'] for row in window)
    span = window[-1]['t_s'] - window[0]['t_s']
    print(f'frames {len(window)} over {span:.1f} s: average {len(window) / span:.1f} FPS, '
          f'median {1000 / percentile(times, 0.5):.1f} FPS, 1% low {1000 / percentile(times, 0.99):.1f} FPS, '
          f'worst {times[-1]:.1f} ms')
    edges = (0.0, *SPREAD_MS, float('inf'))
    parts = []
    for low, high in zip(edges, edges[1:]):
        share = sum(1 for t in times if low <= t < high) / len(times) * 100
        label = f'{low:g}+ ms' if high == float('inf') else f'{low:g}-{high:g} ms'
        parts.append(f'{label} {share:.0f}%')
    print('  frame times: ' + ', '.join(parts))
    for column in COLUMNS:
        values = [row[column] for row in window if column in row]
        if values:
            print(f'  {column:16} median {statistics.median(values):8.2f}  mean {statistics.mean(values):8.2f}')
    return True


def read_threads(path):
    threads = {}
    with open(path) as file:
        for line in file:
            parts = line.rstrip('\n').split('|')
            if len(parts) == 3 and parts[1]:
                threads[parts[2]] = (parts[0], float(parts[1]))
    return threads


def summarize_threads(start_path, end_path, seconds):
    tick = os.sysconf('SC_CLK_TCK')
    start, end = read_threads(start_path), read_threads(end_path)
    busy = sorted(((end[tid][1] - start[tid][1]) / tick / seconds * 100, end[tid][0]) for tid in end if tid in start)
    top = reversed(busy[-TOP_THREADS:])
    print('  threads (CPU % of one core): ' + ', '.join(f'{name} {share:.0f}%' for share, name in top))


def summarize_gpu(path):
    try:
        with open(path) as file:
            samples = sorted(float(line) for line in file if line.strip().replace('.', '', 1).isdigit())
    except OSError:
        return
    if samples:
        print(f'  GPU utilization: median {percentile(samples, 0.5):.0f}%')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('frames')
    parser.add_argument('--ready', action='store_true')
    parser.add_argument('--from', dest='start', type=float, default=0.0)
    parser.add_argument('--seconds', type=float, default=float('inf'))
    parser.add_argument('--threads', nargs=2)
    parser.add_argument('--gpu')
    args = parser.parse_args()
    rows = read_frames(args.frames)
    if args.ready:
        tail = rows[-READY_FRAMES:]
        sys.exit(0 if len(tail) == READY_FRAMES and all(row['draws'] > LEVEL_DRAWS for row in tail) else 1)
    if summarize_frames(rows, args.start, args.seconds) == False:
        sys.exit(1)
    if args.threads is not None:
        summarize_threads(*args.threads, args.seconds)
    if args.gpu is not None:
        summarize_gpu(args.gpu)


if __name__ == '__main__':
    main()
