import os
import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split

# CONSTANTS
PREFIX = "TinyML"
DATASET_FILE = "sensor_data.csv"

# HARDWARE THRESHOLDS FOR NORMALIZATION (Should match C++ DEFAULT_ config)
T_MIN, T_MAX = -6.0, 0.0
H_MIN, H_MAX = 30.0, 50.0

# PATHS
# Navigate one level up (..), then into 'include' folder
OUTPUT_HEADER_PATH = os.path.join("..", "include", f"{PREFIX}.h")

def feature_engineering(df):
    """
    Calculates normalized values and rate of change (speed) for the dataset.
    """
    print("[INFO] [TRAINING] Applying feature engineering...")
    
    # 1. Normalization
    df['t_norm'] = (df['temp'] - T_MIN) / (T_MAX - T_MIN)
    df['h_norm'] = (df['humidity'] - H_MIN) / (H_MAX - H_MIN)
    
    # 2. Rate of Change (Speed)
    # diff() calculates the difference with the previous row. fillna(0) handles the first row.
    df['t_speed'] = df['temp'].diff().fillna(0.0)
    df['h_speed'] = df['humidity'].diff().fillna(0.0)
    
    return df

def train_and_export():
    print(f"[INFO] [TRAINING] Loading dataset from {DATASET_FILE}")
    try:
        data = pd.read_csv(DATASET_FILE)
    except FileNotFoundError:
        print(f"[ERR]  [TRAINING] File {DATASET_FILE} not found. Please run data_collector.py first.")
        return

    # Apply Feature Engineering
    data = feature_engineering(data)

    # Select features (X) and target (y)
    feature_cols = ["t_norm", "h_norm", "t_speed", "h_speed"]
    X = data[feature_cols].values
    y = data["label"].values

    print(f"[INFO] [TRAINING] Dataset shape: {X.shape}. Splitting train/test data...")
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    # Build the Neural Network Model
    print("[INFO] [TRAINING] Building and compiling model...")
    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(4,)),
        tf.keras.layers.Dense(16, activation='relu'),
        tf.keras.layers.Dense(8, activation='relu'),
        tf.keras.layers.Dense(3, activation='softmax') # 3 classes output
    ])

    model.compile(loss="sparse_categorical_crossentropy", 
                  optimizer="adam", 
                  metrics=["accuracy"])
    
    # Train the model
    print("[INFO] [TRAINING] Starting training process...")
    model.fit(X_train, y_train, epochs=100, batch_size=16, validation_data=(X_test, y_test), verbose=1)
    
    # Save the Keras model for backup
    backup_model_path = f"{PREFIX}_backup.h5"
    model.save(backup_model_path)
    print(f"[INFO] [TRAINING] Keras model saved as {backup_model_path}")

    # Convert to TFLite
    print("[INFO] [TRAINING] Converting model to TensorFlow Lite format...")
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    tflite_model = converter.convert()

    # Convert TFLite binary to C array format
    print(f"[INFO] [TRAINING] Exporting model to C-header file at: {OUTPUT_HEADER_PATH}")
    
    hex_lines = [', '.join([f'0x{byte:02x}' for byte in tflite_model[i:i + 12]]) for i in range(0, len(tflite_model), 12)]
    hex_array = ',\n  '.join(hex_lines)

    # Ensure target directory exists
    os.makedirs(os.path.dirname(OUTPUT_HEADER_PATH), exist_ok=True)

    with open(OUTPUT_HEADER_PATH, 'w') as header_file:
        header_file.write(f'// Auto-generated TinyML Model Header\n')
        header_file.write(f'// Inputs: t_norm, h_norm, t_speed, h_speed | Outputs: 3 classes (Normal, Door, Fault)\n\n')
        header_file.write('#ifndef __TINYML_MODEL_H__\n')
        header_file.write('#define __TINYML_MODEL_H__\n\n')
        header_file.write(f'const unsigned int model_len = {len(tflite_model)};\n')
        header_file.write('const unsigned char model[] = {\n  ')
        header_file.write(f'{hex_array}\n')
        header_file.write('};\n\n')
        header_file.write('#endif // __TINYML_MODEL_H__\n')

    print("[INFO] [TRAINING] SUCCESS! The TinyML.h file has been updated.")
    print("[INFO] [TRAINING] You can now Build and Upload your PlatformIO project.")

if __name__ == "__main__":
    train_and_export()