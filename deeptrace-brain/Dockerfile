FROM python:3.9-slim

WORKDIR /app

# Install system dependencies (needed for psycopg2)
RUN apt-get update && apt-get install -y \
    libpq-dev gcc \
    && rm -rf /var/lib/apt/lists/*

# Install Python deps
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy Code & Models
COPY src/ ./src/
COPY models/ ./models/

# Set Python path so imports work
ENV PYTHONPATH=/app/src

# Run the consumer
CMD ["python", "src/traffic_consumer.py"]