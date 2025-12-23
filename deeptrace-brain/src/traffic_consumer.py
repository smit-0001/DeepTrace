import os
import redis
import json
import time
from sqlalchemy import create_engine, Column, Integer, String, Float, DateTime
from sqlalchemy.orm import sessionmaker, declarative_base
from datetime import datetime
from inference_engine import InferenceEngine

# ---------------------------------------------------------
# Configuration
# ---------------------------------------------------------
REDIS_HOST = os.getenv("REDIS_HOST", "localhost")
REDIS_PORT = int(os.getenv("REDIS_PORT", 6379))
DB_URL = os.getenv("DATABASE_URL", "postgresql://admin:admin@db:5432/deeptrace_logs")
MODEL_PATH = os.getenv("MODEL_PATH", "../models/deeptrace_rf_v1.pkl")
# We assume the feature list is in the same folder as the model
FEATURE_PATH = os.path.join(os.path.dirname(MODEL_PATH), "model_features.json")

# ---------------------------------------------------------
# Database Setup (Quick & Dirty for now)
# ---------------------------------------------------------
Base = declarative_base()

class Alert(Base):
    __tablename__ = 'alerts'
    id = Column(Integer, primary_key=True)
    timestamp = Column(DateTime, default=datetime.utcnow)
    src_ip = Column(String)
    dst_ip = Column(String)
    attack_type = Column(String)
    confidence = Column(Float)
    raw_data = Column(String) # Store the JSON just in case

def init_db():
    print(f"🔌 Connecting to Database: {DB_URL}")
    engine = create_engine(DB_URL)
    Base.metadata.create_all(engine)
    return sessionmaker(bind=engine)()

# ---------------------------------------------------------
# Main Consumer Loop
# ---------------------------------------------------------
def run():
    # 1. Initialize DB
    # Retry logic because DB takes time to start
    db_session = None
    for i in range(5):
        try:
            db_session = init_db()
            break
        except Exception:
            print("⏳ Waiting for Database...")
            time.sleep(5)
    
    if not db_session:
        print("❌ Could not connect to DB. Exiting.")
        return

    # 2. Initialize Model
    engine = InferenceEngine(MODEL_PATH, FEATURE_PATH)

    # 3. Connect to Redis
    r = redis.Redis(host=REDIS_HOST, port=REDIS_PORT, decode_responses=True)
    pubsub = r.pubsub()
    pubsub.subscribe('network_traffic')
    
    print("👂 Listening for traffic on channel 'network_traffic'...")

    for message in pubsub.listen():
        if message['type'] == 'message':
            json_data = message['data']
            
            # Predict
            label, confidence = engine.predict(json_data)
            
            # Logic: Only log if it's NOT Benign (or whatever your normal label is)
            # Adjust 'BENIGN' if your dataset uses lowercase or 0
            if label.upper() != "BENIGN": 
                print(f"🚨 ALERT: {label} ({confidence:.2f})")
                
                # Parse JSON again to get IP details for the log
                data_dict = json.loads(json_data)
                
                alert = Alert(
                    src_ip=data_dict.get('src_ip', 'unknown'),
                    dst_ip=data_dict.get('dst_ip', 'unknown'),
                    attack_type=label,
                    confidence=confidence,
                    raw_data=json_data
                )
                db_session.add(alert)
                db_session.commit()
            else:
                # Optional: Print heartbeat for benign traffic
                print(f"✅ Benign traffic detected.")

if __name__ == "__main__":
    run()