"""Optional Python script to plot FIR filter signals from CSV export."""

import sys
from pathlib import Path

def plot_signals(csv_path: str):
    try:
        import matplotlib.pyplot as plt
        import pandas as pd
    except ImportError:
        print("[!] matplotlib or pandas not installed. Install with: pip install matplotlib pandas")
        print("[!] Note: You can open the self-contained HTML report in your browser instead!")
        return

    path = Path(csv_path)
    if not path.exists():
        print(f"[-] CSV file not found: {csv_path}")
        return

    df = pd.read_csv(csv_path)
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)

    ax1.plot(df['time_sec'], df['raw_signal'], color='#fb923c', alpha=0.8, label='Raw Noisy Signal')
    ax1.set_title('Raw Input Signal')
    ax1.set_ylabel('Amplitude')
    ax1.grid(True, linestyle='--', alpha=0.5)
    ax1.legend()

    ax2.plot(df['time_sec'], df['filtered_signal'], color='#38bdf8', linewidth=2.0, label='Filtered Output')
    ax2.set_title('Filtered Signal (FIR Low-Pass)')
    ax2.set_xlabel('Time (seconds)')
    ax2.set_ylabel('Amplitude')
    ax2.grid(True, linestyle='--', alpha=0.5)
    ax2.legend()

    plt.tight_layout()
    output_png = path.with_suffix('.png')
    plt.savefig(output_png, dpi=150)
    print(f"[+] Plot saved to: {output_png}")
    plt.show()

if __name__ == '__main__':
    csv_file = sys.argv[1] if len(sys.argv) > 1 else 'build/signals.csv'
    plot_signals(csv_file)
