from fastapi import FastAPI, Depends, HTTPException
from sqlalchemy import create_engine, Column, Integer, String, Float, DateTime, text
from sqlalchemy.orm import sessionmaker, declarative_base, Session
from datetime import datetime
import os

# Configuration
DATABASE_URL = os.getenv("DATABASE_URL", "postgresql://admin:admin@db:5432/deeptrace_logs")

# Database Setup
Base = declarative_base()
engine = create_engine(DATABASE_URL)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

# Define the Alert Model (Must match the one in traffic_consumer.py)
class Alert(Base):
    __tablename__ = 'alerts'
    id = Column(Integer, primary_key=True, index=True)
    timestamp = Column(DateTime, default=datetime.utcnow)
    src_ip = Column(String)
    dst_ip = Column(String)
    attack_type = Column(String)
    confidence = Column(Float)
    raw_data = Column(String)

# Create Tables (Safe to run multiple times)
Base.metadata.create_all(bind=engine)

# FastAPI App
app = FastAPI(title="DeepTrace API")

# Dependency
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()

@app.get("/")
def health_check():
    return {"status": "online", "service": "DeepTrace API"}

@app.get("/alerts")
def get_alerts(limit: int = 50, db: Session = Depends(get_db)):
    """Fetch recent alerts from the database"""
    alerts = db.query(Alert).order_by(Alert.timestamp.desc()).limit(limit).all()
    return alerts

@app.get("/stats")
def get_stats(db: Session = Depends(get_db)):
    """Get simple stats for the dashboard"""
    total_alerts = db.query(Alert).count()
    
    # Raw SQL for faster aggregation
    try:
        top_attack = db.execute(text("SELECT attack_type, COUNT(*) as c FROM alerts GROUP BY attack_type ORDER BY c DESC LIMIT 1")).fetchone()
        most_frequent = top_attack[0] if top_attack else "None"
    except:
        most_frequent = "N/A"

    return {
        "total_alerts": total_alerts,
        "most_frequent_attack": most_frequent
    }