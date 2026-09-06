#!/usr/bin/env python3
"""Generate presentation-ready figures from the H3 dataset + trained classifier.

Reuses load_dataset()/extract_features()/FEATURE_NAMES from
train_h3_classifier.py so the plotted numbers are guaranteed to match that
script's reported accuracy/tree -- this file only visualises, it does not
re-derive the model.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import matplotlib

matplotlib.rcParams["font.sans-serif"] = [
    "Microsoft JhengHei", "Microsoft YaHei", "SimHei", "Arial Unicode MS",
]
matplotlib.rcParams["axes.unicode_minus"] = False

import matplotlib.pyplot as plt
import numpy as np
from sklearn.metrics import confusion_matrix
from sklearn.model_selection import LeaveOneOut, cross_val_predict
from sklearn.tree import DecisionTreeClassifier, plot_tree

sys.path.insert(0, str(Path(__file__).resolve().parent))
from train_h3_classifier import FEATURE_NAMES, load_dataset  # noqa: E402

TINYML_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = TINYML_ROOT / "data" / "analysis"
LABEL_ORDER = ["NO_CONTACT", "NORMAL_GRIP", "OVERLOAD_JAM"]
LABEL_COLORS = {"NO_CONTACT": "#4f8ef7", "NORMAL_GRIP": "#22b8a6", "OVERLOAD_JAM": "#e0574f"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="產生 H3 分類器的簡報用圖表")
    parser.add_argument("--dataset", type=Path, required=True)
    parser.add_argument("--max-depth", type=int, default=3)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    return parser.parse_args()


def plot_feature_scatter(X: np.ndarray, y: np.ndarray, output_path: Path) -> None:
    delta_mean = X[:, FEATURE_NAMES.index("delta_mean")]
    last_mean = X[:, FEATURE_NAMES.index("last_mean")]
    last_std = X[:, FEATURE_NAMES.index("last_std")]

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.5))
    ax = axes[0]
    for label in LABEL_ORDER:
        mask = y == label
        ax.scatter(delta_mean[mask], last_mean[mask], label=label, color=LABEL_COLORS[label],
                   s=70, alpha=0.85, edgecolor="white", linewidth=0.6)
    ax.axvline(16.04, color="#8fa0bd", linestyle="--", linewidth=1.2, label="決策樹門檻 (delta_mean=16.04)")
    ax.set_xlabel("delta_mean（末視窗減首視窗電流均值，raw ADC）")
    ax.set_ylabel("last_mean（末視窗電流均值，raw ADC）")
    ax.set_title("首/末視窗電流變化：為何可分")
    ax.legend(fontsize=8, loc="best")
    ax.grid(alpha=0.25)

    ax2 = axes[1]
    box_data = [last_std[y == label] for label in LABEL_ORDER]
    bp = ax2.boxplot(box_data, tick_labels=LABEL_ORDER, patch_artist=True)
    for patch, label in zip(bp["boxes"], LABEL_ORDER):
        patch.set_facecolor(LABEL_COLORS[label])
        patch.set_alpha(0.6)
    ax2.set_ylabel("last_std（末視窗電流標準差，raw ADC）")
    ax2.set_title("末視窗電流「漣波」：馬達是否仍在正常運轉")
    ax2.grid(alpha=0.25, axis="y")

    fig.suptitle("H3 過載/卡料分類：關鍵特徵分佈（30 筆真實硬體 trial）", fontsize=12)
    fig.tight_layout()
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def plot_confusion(y: np.ndarray, y_pred: np.ndarray, output_path: Path) -> None:
    cm = confusion_matrix(y, y_pred, labels=LABEL_ORDER)
    fig, ax = plt.subplots(figsize=(5, 4.5))
    im = ax.imshow(cm, cmap="Blues")
    ax.set_xticks(range(len(LABEL_ORDER)))
    ax.set_yticks(range(len(LABEL_ORDER)))
    ax.set_xticklabels(LABEL_ORDER, rotation=20, ha="right")
    ax.set_yticklabels(LABEL_ORDER)
    ax.set_xlabel("預測")
    ax.set_ylabel("實際")
    ax.set_title("Leave-one-out 混淆矩陣（30 筆 trial，準確率 80%）")
    for i in range(len(LABEL_ORDER)):
        for j in range(len(LABEL_ORDER)):
            ax.text(j, i, str(cm[i, j]), ha="center", va="center",
                    color="white" if cm[i, j] > cm.max() / 2 else "black", fontsize=13)
    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    fig.tight_layout()
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def plot_tree_diagram(clf: DecisionTreeClassifier, output_path: Path) -> None:
    fig, ax = plt.subplots(figsize=(10, 6))
    plot_tree(clf, feature_names=FEATURE_NAMES, class_names=list(clf.classes_),
              filled=True, rounded=True, fontsize=10, ax=ax)
    ax.set_title("H3 決策樹（max_depth=3，完整 30 筆資料訓練，已上韌體）")
    fig.tight_layout()
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def pick_representative_trial(trials: list[dict], label: str) -> dict:
    """The trial whose delta_mean is closest to that label's median -- a
    "first match" pick can land on an outlier trial that doesn't visually
    read as typical for its class; median-delta_mean is a simple stand-in for
    "representative" without pulling in sklearn just for one plot."""
    candidates = [t for t in trials if t["label"] == label]
    deltas = []
    for t in candidates:
        windows = t["windows"]
        first_mean = np.mean(windows[0]["acs712_adc_raw"])
        last_mean = np.mean(windows[-1]["acs712_adc_raw"])
        deltas.append(last_mean - first_mean)
    median_delta = float(np.median(deltas))
    closest_index = int(np.argmin([abs(d - median_delta) for d in deltas]))
    return candidates[closest_index]


def plot_example_traces(dataset_path: Path, output_path: Path) -> None:
    with dataset_path.open(encoding="utf-8") as f:
        payload = json.load(f)
    trials = payload["trials"]

    fig, axes = plt.subplots(1, 3, figsize=(13, 3.8), sharey=True)
    for ax, label in zip(axes, LABEL_ORDER):
        trial = pick_representative_trial(trials, label)
        windows = trial["windows"]
        combined = np.concatenate([np.asarray(w["acs712_adc_raw"], dtype=float) for w in windows])
        sample_rate_hz = windows[0].get("sample_rate_hz", 974.0)
        t = np.arange(len(combined)) / sample_rate_hz
        first_window_len = len(windows[0]["acs712_adc_raw"])
        boundary_s = first_window_len / sample_rate_hz

        last_window = np.asarray(windows[-1]["acs712_adc_raw"], dtype=float)
        last_std = float(np.std(last_window))

        ax.axvspan(boundary_s, t[-1], color=LABEL_COLORS[label], alpha=0.08)
        ax.plot(t, combined, color=LABEL_COLORS[label], linewidth=0.6)
        ax.axvline(boundary_s, color="gray", linestyle=":", linewidth=1)
        ax.set_title(f"{label}\n末視窗 std={last_std:.1f}", fontsize=11)
        ax.set_xlabel("時間 (s)")
        ax.grid(alpha=0.2)
    axes[0].set_ylabel("ACS712 raw ADC")
    fig.suptitle("H3 範例 trial：原始電流波形（虛線=首/末視窗分界，網底=末視窗）", fontsize=12)
    fig.tight_layout()
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def main() -> int:
    args = parse_args()
    X, y = load_dataset(args.dataset)

    clf = DecisionTreeClassifier(max_depth=args.max_depth, random_state=0)
    loo = LeaveOneOut()
    y_pred = cross_val_predict(clf, X, y, cv=loo)
    clf.fit(X, y)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    plot_feature_scatter(X, y, args.output_dir / "h3_feature_scatter.png")
    plot_confusion(y, y_pred, args.output_dir / "h3_confusion_matrix.png")
    plot_tree_diagram(clf, args.output_dir / "h3_decision_tree.png")
    plot_example_traces(args.dataset, args.output_dir / "h3_example_traces.png")
    print(f"四張圖已存到：{args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
