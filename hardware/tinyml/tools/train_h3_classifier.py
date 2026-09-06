#!/usr/bin/env python3
"""Train a small decision tree that classifies EEP gripper trials (NORMAL_GRIP /
OVERLOAD_JAM / NO_CONTACT) from ACS712 current features, using the dataset
exported by live-dashboard's H3 capture flow (index.html "下載資料集 (JSON)").

Trains on raw ADC counts, not amps: the divider ratio / zero-current offset
documented in eep_gripper_h1.ino are ACS712 datasheet defaults, not multimeter-
measured. An unverified affine transform of the raw signal adds no separating
information a tree can't already find in raw counts, and keeping thresholds in
the same domain the firmware already streams avoids a second calibration-
dependent conversion between training and on-device inference.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from sklearn.metrics import classification_report, confusion_matrix
from sklearn.model_selection import LeaveOneOut, cross_val_predict
from sklearn.tree import DecisionTreeClassifier, export_text

TINYML_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = TINYML_ROOT / "data" / "analysis"
FEATURE_NAMES = ["first_mean", "last_mean", "delta_mean", "last_std", "last_ptp"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Train H3 決策樹分類器 (NORMAL_GRIP / OVERLOAD_JAM / NO_CONTACT)"
    )
    parser.add_argument("--dataset", type=Path, required=True, help="live-dashboard 匯出的 eep_gripper_dataset_*.json")
    parser.add_argument("--max-depth", type=int, default=3, help="決策樹最大深度（預設 3，方便手動搬進韌體）")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    return parser.parse_args()


def extract_features(windows: list[dict]) -> np.ndarray:
    """Per-trial features contrasting the FIRST vs LAST streamed window.

    A single trial's concatenated-burst stats (mean/RMS/crest factor over the
    whole clip) turned out to classify barely better than chance (~60% LOOCV
    across 3 classes): they average away the one thing that actually separates
    NO_CONTACT / NORMAL_GRIP / OVERLOAD_JAM here, which is *how the current
    changes over the course of the sweep*, not its overall level. Whether the
    paw is closing on nothing, something compressible, or something that stalls
    it shows up as how much the mean current rises from first window to last
    (delta_mean), and how much ripple remains in the last window (last_std --
    low ripple when the motor has stalled against a jam, vs. still-moving
    ripple when it is smoothly closing on a normal object). Using first/last
    (not a hardcoded window[0]/window[1]) keeps this valid if a future capture
    run's sweep produces more than 2 windows per trial.
    """
    first = np.asarray(windows[0]["acs712_adc_raw"], dtype=float)
    last = np.asarray(windows[-1]["acs712_adc_raw"], dtype=float)
    first_mean = float(np.mean(first))
    last_mean = float(np.mean(last))
    delta_mean = last_mean - first_mean
    last_std = float(np.std(last))
    last_ptp = float(np.ptp(last))
    return np.array([first_mean, last_mean, delta_mean, last_std, last_ptp])


def load_dataset(path: Path) -> tuple[np.ndarray, np.ndarray]:
    with path.open(encoding="utf-8") as f:
        payload = json.load(f)
    trials = payload.get("trials", [])
    if not trials:
        raise RuntimeError(f"{path} 沒有任何 trial")

    features = []
    labels = []
    for trial in trials:
        windows = trial.get("windows", [])
        if not windows:
            print(f"警告：一筆 {trial.get('label')} trial 沒有任何 window，已略過", file=sys.stderr)
            continue
        features.append(extract_features(windows))
        labels.append(trial["label"])
    return np.vstack(features), np.array(labels)


def main() -> int:
    args = parse_args()
    X, y = load_dataset(args.dataset)
    print(f"載入 {len(y)} 筆 trial，特徵 shape={X.shape}")
    classes, counts = np.unique(y, return_counts=True)
    print("類別分布:", dict(zip(classes.tolist(), counts.tolist())))

    clf = DecisionTreeClassifier(max_depth=args.max_depth, random_state=0)

    # Leave-one-out CV: with only ~10 trials/class, this is the honest way to
    # estimate generalisation without permanently giving up a held-out split's
    # worth of already-scarce data to a fixed test set.
    loo = LeaveOneOut()
    y_pred = cross_val_predict(clf, X, y, cv=loo)
    print("\n=== Leave-one-out 交叉驗證 ===")
    print(classification_report(y, y_pred, zero_division=0))
    labels_sorted = sorted(classes.tolist())
    cm = confusion_matrix(y, y_pred, labels=labels_sorted)
    print("混淆矩陣 (rows=實際, cols=預測):")
    print(" " * 15 + "".join(f"{l:>14s}" for l in labels_sorted))
    for label, row in zip(labels_sorted, cm):
        print(f"{label:15s}" + "".join(f"{v:>14d}" for v in row))

    # Fit on the full dataset for the tree that actually gets reported/ported --
    # LOOCV above already gives the honest accuracy estimate for trees of this
    # same max_depth; this final fit just picks one concrete tree to report.
    clf.fit(X, y)
    tree_text = export_text(clf, feature_names=FEATURE_NAMES)
    print(f"\n=== 完整資料訓練出的決策樹 (max_depth={args.max_depth}) ===")
    print(tree_text)
    print("特徵重要性:")
    for name, importance in sorted(zip(FEATURE_NAMES, clf.feature_importances_), key=lambda x: -x[1]):
        print(f"  {name:15s} {importance:.3f}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    report_path = args.output_dir / "h3_decision_tree.txt"
    report_path.write_text(tree_text, encoding="utf-8")
    print(f"\n決策樹文字規則已存成：{report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
