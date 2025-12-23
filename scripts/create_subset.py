import pandas as pd
import os

# Configuration
INPUT_FILE = "../data/processed/cleaned_ids2017.csv"
OUTPUT_FILE = "../data/processed/subset_ids2017.csv"
SAMPLE_SIZE = 50000  # 100k rows is plenty for testing pipeline

def create_subset():
    print(f"⏳ Reading large file: {INPUT_FILE}...")
    
    if not os.path.exists(INPUT_FILE):
        print(f"❌ Error: File not found at {INPUT_FILE}")
        return

    # Read the file
    # If the file is massive, we can just read a sample directly without loading all to RAM
    # But since you likely have 8GB+ RAM, loading and sampling is easiest.
    try:
        df = pd.read_csv(INPUT_FILE)
        print(f"✅ Loaded {len(df)} rows.")
        
        if len(df) > SAMPLE_SIZE:
            print(f"🎲 Sampling {SAMPLE_SIZE} random rows...")
            df_subset = df.sample(n=SAMPLE_SIZE, random_state=42)
        else:
            print("⚠️ Data is smaller than sample size. Using full dataset.")
            df_subset = df

        print(f"💾 Saving to {OUTPUT_FILE}...")
        df_subset.to_csv(OUTPUT_FILE, index=False)
        print("✅ Done! You can now train on the subset.")

    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    create_subset()