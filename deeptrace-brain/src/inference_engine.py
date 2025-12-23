import joblib
import pandas as pd
import json
import os
import numpy as np

class InferenceEngine:
    def __init__(self, model_path, feature_list_path):
        self.model = None
        self.features = []
        self._load_model(model_path, feature_list_path)

    def _load_model(self, model_path, feature_list_path):
        print(f"🧠 Loading model from {model_path}...")
        try:
            self.model = joblib.load(model_path)
            
            # Load the exact feature order used during training
            with open(feature_list_path, 'r') as f:
                self.features = json.load(f)
            
            print(f"✅ Model loaded. Expecting {len(self.features)} features.")
        except Exception as e:
            print(f"❌ Error loading model: {e}")
            raise e

    def predict(self, json_data):
        """
        Input: JSON string from C++ Sniffer
        Output: Prediction (String), Confidence (Float), Attack Type (String)
        """
        try:
            data_dict = json.loads(json_data)
            
            # 1. Convert dictionary to DataFrame with correct column order
            # We only select the features the model knows about. 
            # C++ might send extra metadata (IPs, timestamps) that we ignore here.
            input_df = pd.DataFrame([data_dict])
            
            # Fill missing cols with 0 (safety net)
            for f in self.features:
                if f not in input_df.columns:
                    input_df[f] = 0
            
            # Reorder columns to match training
            X = input_df[self.features]
            
            # 2. Predict
            prediction_idx = self.model.predict(X)[0]
            confidence = np.max(self.model.predict_proba(X))
            
            # Mapping (assuming your model output encoded integers, 
            # if it outputs strings directly, you don't need a map)
            # You might need to adjust this based on your specific LabelEncoder
            # For now, we assume the model returns the string label directly
            label = str(prediction_idx) 
            
            return label, float(confidence)

        except Exception as e:
            print(f"⚠️ Prediction Error: {e}")
            return "Error", 0.0