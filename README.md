# ManDiligMa
**Smart Irrigation and Health Monitoring System Using Machine Learning and Mobile Application**

ManDiligMa is an intelligent irrigation system designed to optimize water usage for lettuce cultivation. By leveraging the **K-Nearest Neighbors (KNN)** algorithm, the system analyzes real-time environmental data **Temperature, Humidity, and Soil Moisture** to accurately determine if irrigation is required.

---

## How It Works
The system operates in two main phases:

1. **Model Training:** The system uses the **KNN algorithm** trained on historical environmental data. KNN works by identifying the "K" most similar historical instances to the current sensor readings to classify whether the plant needs water.
2. **Real-Time Prediction:**
    * **Sensors:** An Arduino Uno captures live data from Soil Moisture, Temperature, and Humidity sensors.
    * **Processing:** The Python script receives this data via Serial communication.
    * **Classification:** The KNN model evaluates the live input and outputs a decision: **Irrigate** or **Do Not Irrigate**.

---

## Key Features
* **KNN-Powered Logic:** Replaces static "if-else" thresholds with a machine learning model that understands the relationship between different environmental factors.
* **Precision Monitoring:** Specifically tuned for lettuce, ensuring the soil moisture is kept at an optimal level.
* **Data Cleaning:** Built-in preprocessing to handle missing values and scale features for better KNN accuracy.
* **Model Persistence:** The trained model is saved as `irrigation_model.pkl` for fast deployment without retraining.

---

## Tech Stack
* **Hardware:** Arduino Uno, DHT22 (Temp/Humid), Soil Moisture Sensor, Relay.
* **Language:** Python 3.6+
* **Libraries:** * `scikit-learn`: For the KNN implementation.
    * `pandas` & `numpy`: For data manipulation.
    * `pyserial`: For Arduino-to-PC communication.
    * `joblib`: For saving/loading the ML model.

---

## Getting Started

### 1. Prerequisites
Ensure you have Python installed. Then, install the required libraries:
```bash
pip install pandas scikit-learn numpy joblib pyserial
