FROM python:3.9-slim

WORKDIR /app

# Install system dependencies (for Postgres)
RUN apt-get update && apt-get install -y \
    libpq-dev gcc \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app/ ./app/

# Expose port 8000
EXPOSE 8000

# Run with Uvicorn (Production Server)
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]