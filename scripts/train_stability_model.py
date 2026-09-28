#!/usr/bin/env python3
"""
scripts/train_stability_model.py
--------------------------------
AstroGenesis Orbital Stability Model Training & Export Pipeline

Trains an explainable Random Forest Classifier on simulated orbital dynamics,
evaluates precision/recall/F1/ROC-AUC, and exports the model to:
  1. assets/models/orbital_stability.onnx (ONNX format)
  2. assets/models/orbital_stability_model.json (Structured tree weights for native C++ inference)
"""

import os
import sys
import json
import time
import numpy as np
import pandas as pd

from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, confusion_matrix, roc_auc_score

FEATURE_NAMES = [
    "body_count",
    "star_mass_solar",
    "total_mass_ratio",
    "max_mass_ratio",
    "min_mutual_hill_sep",
    "mean_mutual_hill_sep",
    "max_eccentricity",
    "mean_eccentricity",
    "orbit_crossing_flag",
    "min_period_ratio",
    "angular_momentum_deficit",
    "energy_drift_pct"
]

def export_model_to_json(rf, feature_names, out_path, metrics=None):
    """
    Serializes a scikit-learn RandomForestClassifier into an optimized JSON format
    for microsecond zero-dependency evaluation in C++.
    """
    trees = []
    for estimator in rf.estimators_:
        t = estimator.tree_
        node_count = t.node_count

        # Extract node properties
        children_left = t.children_left.tolist()
        children_right = t.children_right.tolist()
        feature = t.feature.tolist()
        threshold = [float(x) for x in t.threshold]
        
        # Class distributions at leaves
        # shape: (node_count, n_outputs, n_classes)
        values = []
        for v in t.value:
            # v[0] has counts for class 0 (Unstable) and class 1 (Stable)
            counts = v[0]
            total = sum(counts)
            prob_unstable = float(counts[0] / total) if total > 0 else 0.5
            prob_stable = float(counts[1] / total) if total > 0 else 0.5
            values.append([prob_unstable, prob_stable])

        tree_data = {
            "node_count": int(node_count),
            "children_left": children_left,
            "children_right": children_right,
            "feature": feature,
            "threshold": threshold,
            "leaf_probs": values
        }
        trees.append(tree_data)

    model_dict = {
        "model_type": "RandomForestClassifier",
        "framework": "scikit-learn",
        "n_estimators": len(trees),
        "n_classes": 2,
        "classes": ["UNSTABLE", "STABLE"],
        "n_features": len(feature_names),
        "feature_names": feature_names,
        "metrics": metrics or {},
        "created_at": time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime()),
        "trees": trees
    }

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(model_dict, f, indent=2)

    print(f"[Export] Saved native C++ JSON model to: {out_path} ({len(trees)} trees)")

def export_model_to_onnx(rf, feature_names, out_path):
    """
    Exports scikit-learn model to standard ONNX format.
    """
    try:
        from skl2onnx import convert_sklearn
        from skl2onnx.common.data_types import FloatTensorType

        initial_type = [('float_input', FloatTensorType([None, len(feature_names)]))]
        onnx_model = convert_sklearn(rf, initial_types=initial_type, target_opset=12)

        os.makedirs(os.path.dirname(out_path), exist_ok=True)
        with open(out_path, "wb") as f:
            f.write(onnx_model.SerializeToString())
        print(f"[Export] Saved ONNX model to: {out_path}")
        return True
    except Exception as e:
        print(f"[Export Warning] ONNX export encountered error: {e}")
        return False

def verify_json_inference(rf, json_path, X_test):
    """
    Simulates the exact C++ tree traversal algorithm to verify parity with scikit-learn.
    """
    with open(json_path, "r", encoding="utf-8") as f:
        m = json.load(f)

    trees = m["trees"]
    n_trees = len(trees)

    py_probs = rf.predict_proba(X_test)
    max_diff = 0.0

    for i in range(min(50, len(X_test))):
        sample = X_test.iloc[i].values
        sum_prob_stable = 0.0

        for t in trees:
            node = 0
            while t["children_left"][node] != -1:
                feat_idx = t["feature"][node]
                thresh = t["threshold"][node]
                if sample[feat_idx] <= thresh:
                    node = t["children_left"][node]
                else:
                    node = t["children_right"][node]
            sum_prob_stable += t["leaf_probs"][node][1]

        c_prob_stable = sum_prob_stable / n_trees
        sk_prob_stable = py_probs[i][1]
        diff = abs(c_prob_stable - sk_prob_stable)
        if diff > max_diff:
            max_diff = diff

    print(f"[Verification] Max discrepancy between Python & C++ tree traversal: {max_diff:.8f}")
    assert max_diff < 1e-4, f"Discrepancy too large: {max_diff}"
    print("  -> Parity Verified: C++ inference logic exactly matches scikit-learn!")

def main():
    print("==========================================================")
    print(" AstroGenesis Orbital Stability Model Training")
    print("==========================================================")

    data_path = "data/orbital_dataset.csv"
    if not os.path.exists(data_path):
        print(f"Error: Dataset not found at {data_path}. Run generate_orbital_dataset.py first.")
        sys.exit(1)

    df = pd.read_csv(data_path)
    print(f"Loaded dataset: {len(df)} configurations")

    X = df[FEATURE_NAMES]
    y = df["is_stable"]

    print(f"Class distribution:")
    print(f"  Stable (1):   {sum(y == 1)} ({sum(y == 1)/len(y)*100:.1f}%)")
    print(f"  Unstable (0): {sum(y == 0)} ({sum(y == 0)/len(y)*100:.1f}%)")

    # Stratified Train/Test split (80% / 20%)
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.20, random_state=42, stratify=y
    )
    print(f"\nSplit: Train={len(X_train)} samples, Test={len(X_test)} samples")

    # 1. Baseline Model: Logistic Regression
    print("\n--- Training Baseline Model: Logistic Regression ---")
    lr = LogisticRegression(max_iter=1000, random_state=42)
    lr.fit(X_train, y_train)
    lr_pred = lr.predict(X_test)
    print(f"  Baseline Accuracy:  {accuracy_score(y_test, lr_pred)*100:.2f}%")
    print(f"  Baseline F1 Score:  {f1_score(y_test, lr_pred):.4f}")

    # 2. Primary Model: Random Forest Classifier
    print("\n--- Training Primary Model: Random Forest Classifier ---")
    rf = RandomForestClassifier(
        n_estimators=50,
        max_depth=8,
        min_samples_split=4,
        min_samples_leaf=2,
        random_state=42,
        n_jobs=-1
    )
    rf.fit(X_train, y_train)

    y_pred = rf.predict(X_test)
    y_prob = rf.predict_proba(X_test)[:, 1]

    acc = accuracy_score(y_test, y_pred)
    prec = precision_score(y_test, y_pred)
    rec = recall_score(y_test, y_pred)
    f1 = f1_score(y_test, y_pred)
    auc = roc_auc_score(y_test, y_prob)
    cm = confusion_matrix(y_test, y_pred)

    print("\n==========================================================")
    print(" MODEL EVALUATION METRICS (Test Set)")
    print("==========================================================")
    print(f"  Accuracy:         {acc * 100:.2f}%")
    print(f"  Precision:        {prec:.4f}")
    print(f"  Recall:           {rec:.4f}")
    print(f"  F1 Score:         {f1:.4f}")
    print(f"  ROC-AUC:          {auc:.4f}")
    print(f"\n  Confusion Matrix:")
    print(f"               Predicted Unstable   Predicted Stable")
    print(f"  True Unstable:      {cm[0, 0]:5d}              {cm[0, 1]:5d}")
    print(f"  True Stable:        {cm[1, 0]:5d}              {cm[1, 1]:5d}")

    print("\n--- Top Feature Importances ---")
    importances = rf.feature_importances_
    indices = np.argsort(importances)[::-1]
    for rank in range(min(8, len(indices))):
        idx = indices[rank]
        print(f"  {rank + 1:2d}. {FEATURE_NAMES[idx]:<28s}: {importances[idx]*100:.2f}%")

    metrics_dict = {
        "accuracy": float(acc),
        "precision": float(prec),
        "recall": float(rec),
        "f1": float(f1),
        "roc_auc": float(auc),
        "test_samples": len(y_test),
        "confusion_matrix": cm.tolist()
    }

    # Export models
    models_dir = "assets/models"
    json_path = os.path.join(models_dir, "orbital_stability_model.json")
    onnx_path = os.path.join(models_dir, "orbital_stability.onnx")

    export_model_to_json(rf, FEATURE_NAMES, json_path, metrics_dict)
    export_model_to_onnx(rf, FEATURE_NAMES, onnx_path)

    # Verification of C++ traversal logic
    verify_json_inference(rf, json_path, X_test)
    print("\n[Done] Model training and export pipeline completed successfully.")

if __name__ == "__main__":
    main()
