#!/usr/bin/env python3
"""Summarise H0 vibration windows and plot run-level spectra for human review.

This script deliberately reports measurements only. It never decides whether H0
passed: the normal-versus-loaded comparison is an explicit human review gate.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from scipy.signal import periodogram


WINDOW_SAMPLES = 1024
BANDS_HZ = ((5, 50), (50, 150), (150, 300), (300, 500))
AXES = ("ax_g", "ay_g", "az_g")
TINYML_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = TINYML_ROOT / "data" / "analysis" / "baseline_spectra.png"


@dataclass
class RunResult:
    path: Path
    label: str
    time_features: dict[str, np.ndarray]
    band_features: dict[str, np.ndarray]
    frequency_hz: np.ndarray
    mean_power: np.ndarray


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Analyse normal/loaded TinyML H0 JSONL runs.")
    parser.add_argument("--files", nargs="+", required=True, type=Path, help="One JSONL path per run")
    parser.add_argument(
        "--labels",
        nargs="+",
        help="Optional labels parallel to --files; otherwise infer normal/loaded from filename.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Spectrum PNG path (default: {DEFAULT_OUTPUT})",
    )
    args = parser.parse_args()
    if args.labels is not None and len(args.labels) != len(args.files):
        parser.error("--labels must provide exactly one label for every --files path")
    return args


def infer_label(path: Path) -> str:
    name = path.stem.lower()
    if "normal" in name:
        return f"normal ({path.stem})"
    if "loaded" in name or "load" in name:
        return f"loaded ({path.stem})"
    return f"unlabelled ({path.stem})"


def load_windows(path: Path) -> list[dict]:
    windows: list[dict] = []
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise RuntimeError(f"無法讀取 {path}: {exc}") from exc

    for line_number, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            print(f"警告：{path}:{line_number} 不是 JSON，已略過", file=sys.stderr)
            continue
        if payload.get("type") != "window":
            continue
        if not all(axis in payload for axis in AXES):
            print(f"警告：{path}:{line_number} 缺少加速度軸，已略過", file=sys.stderr)
            continue
        if not isinstance(payload.get("sample_rate_hz"), (int, float)) or payload["sample_rate_hz"] <= 0:
            print(f"警告：{path}:{line_number} sample_rate_hz 無效，已略過", file=sys.stderr)
            continue
        if any(len(payload[axis]) != WINDOW_SAMPLES for axis in AXES):
            print(f"警告：{path}:{line_number} 不是 {WINDOW_SAMPLES} 點視窗，已略過", file=sys.stderr)
            continue
        windows.append(payload)
    return windows


def calculate_time_features(values: np.ndarray) -> tuple[float, float, float]:
    """Return RMS, peak-to-peak, and crest factor; peak means max absolute value.

    RMS and crest factor are computed on the mean-removed (AC-coupled) signal.
    A near-vertical axis carries a ~1 g gravity offset; leaving that DC term in
    would make RMS/crest factor mostly reflect mounting orientation instead of
    vibration amplitude, masking exactly the normal-vs-loaded difference H0 is
    checking for. Peak-to-peak already cancels a constant offset by definition,
    so mean removal does not change it, but it is applied for consistency.
    """
    ac_values = values - np.mean(values)
    rms = float(np.sqrt(np.mean(np.square(ac_values))))
    peak_to_peak = float(np.ptp(ac_values))
    peak = float(np.max(np.abs(ac_values)))
    crest_factor = peak / rms if rms > 0 else np.nan
    return rms, peak_to_peak, crest_factor


def band_power(frequency_hz: np.ndarray, psd: np.ndarray, low_hz: float, high_hz: float) -> float:
    mask = (frequency_hz >= low_hz) & (frequency_hz < high_hz)
    if np.count_nonzero(mask) < 2:
        return np.nan
    return float(np.trapz(psd[mask], frequency_hz[mask]))


def analyse_run(path: Path, label: str) -> RunResult:
    windows = load_windows(path)
    if not windows:
        raise RuntimeError(f"{path} 找不到有效的完整 window")

    time_values: dict[str, list[tuple[float, float, float]]] = {axis: [] for axis in AXES}
    band_values: dict[str, list[list[float]]] = {axis: [] for axis in AXES}
    spectra: list[tuple[np.ndarray, np.ndarray]] = []

    for payload in windows:
        sample_rate_hz = float(payload["sample_rate_hz"])
        axis_psds: list[np.ndarray] = []
        frequencies: np.ndarray | None = None
        for axis in AXES:
            values = np.asarray(payload[axis], dtype=float)
            time_values[axis].append(calculate_time_features(values))
            # All three axes are analysed instead of assuming az is always the motor's
            # dominant direction: sensor mounting orientation can differ between rigs.
            # The plotted spectrum is their mean PSD, so run comparisons stay compact.
            frequency_hz, psd = periodogram(
                values,
                fs=sample_rate_hz,
                window="hann",
                detrend="constant",
                scaling="density",
            )
            frequencies = frequency_hz
            axis_psds.append(psd)
            band_values[axis].append(
                [band_power(frequency_hz, psd, low, high) for low, high in BANDS_HZ]
            )
        assert frequencies is not None
        spectra.append((frequencies, np.mean(np.vstack(axis_psds), axis=0)))

    # Sample rates are recorded per window, so frequency bins can differ slightly.
    # Interpolate each to the run's common range before averaging their PSDs.
    maximum_common_hz = min(float(frequency[-1]) for frequency, _ in spectra)
    common_frequency_hz = np.linspace(0.0, maximum_common_hz, WINDOW_SAMPLES // 2 + 1)
    interpolated = [np.interp(common_frequency_hz, frequency, psd) for frequency, psd in spectra]

    return RunResult(
        path=path,
        label=label,
        time_features={axis: np.asarray(values) for axis, values in time_values.items()},
        band_features={axis: np.asarray(values) for axis, values in band_values.items()},
        frequency_hz=common_frequency_hz,
        mean_power=np.mean(np.vstack(interpolated), axis=0),
    )


def format_mean_std(values: np.ndarray) -> str:
    mean = float(np.nanmean(values))
    std = float(np.nanstd(values, ddof=0))
    return f"{mean:.6g} +/- {std:.3g}"


def print_run_summary(result: RunResult) -> None:
    print(f"\nRun: {result.label}")
    print(f"檔案: {result.path}")
    window_count = len(next(iter(result.time_features.values())))
    print(f"有效 window: {window_count}")
    print("時域特徵（跨 window：平均 +/- 標準差）")
    print(f"{'axis':<6} {'RMS (g)':<22} {'peak-to-peak (g)':<22} {'crest factor':<22}")
    for axis in AXES:
        feature_array = result.time_features[axis]
        print(
            f"{axis:<6} {format_mean_std(feature_array[:, 0]):<22} "
            f"{format_mean_std(feature_array[:, 1]):<22} {format_mean_std(feature_array[:, 2]):<22}"
        )

    print("頻帶功率 g^2（跨 window：平均 +/- 標準差）")
    print(f"{'band (Hz)':<14} {'ax_g':<22} {'ay_g':<22} {'az_g':<22}")
    for index, (low_hz, high_hz) in enumerate(BANDS_HZ):
        print(
            f"{low_hz}-{high_hz:<9} "
            f"{format_mean_std(result.band_features['ax_g'][:, index]):<22} "
            f"{format_mean_std(result.band_features['ay_g'][:, index]):<22} "
            f"{format_mean_std(result.band_features['az_g'][:, index]):<22}"
        )


def plot_spectra(results: list[RunResult], output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.figure(figsize=(12, 7))
    for result in results:
        mask = (result.frequency_hz >= 5) & (result.frequency_hz <= 500)
        plt.plot(result.frequency_hz[mask], result.mean_power[mask], linewidth=1.2, label=result.label)
    plt.xlabel("Frequency (Hz)")
    plt.ylabel("Mean PSD across axes (g²/Hz)")
    plt.title("H0 vibration spectra: normal versus loaded runs")
    plt.grid(True, alpha=0.3)
    plt.legend(fontsize=8)
    plt.tight_layout()
    plt.savefig(output_path, dpi=160)
    plt.close()


def main() -> int:
    args = parse_args()
    labels = args.labels if args.labels is not None else [infer_label(path) for path in args.files]
    results: list[RunResult] = []

    for path, label in zip(args.files, labels):
        try:
            results.append(analyse_run(path, label))
        except RuntimeError as exc:
            print(f"錯誤：{exc}", file=sys.stderr)
            return 2

    for result in results:
        print_run_summary(result)
    plot_spectra(results, args.output)
    print(f"\n頻譜圖已存成：{args.output}")
    print(
        "請人工比對上圖：normal run 與 loaded run 的頻譜在哪些頻帶有明顯差異"
        "（建議差異達 2 倍以上功率視為可分）；若完全重疊則 H0 未通過，"
        "不建議往下做 RandomForest。"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
