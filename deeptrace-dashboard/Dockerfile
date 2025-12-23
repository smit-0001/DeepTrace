# Use Node.js environment
FROM node:18-alpine

WORKDIR /app

# Copy package files
COPY package.json ./

# Install dependencies
RUN npm install

# Copy source code
COPY . .

# Expose Vite port
EXPOSE 3000

# Run in development mode (host 0.0.0.0 is crucial for Docker)
# CMD ["npm", "run", "dev", "--", "--host", "0.0.0.0"]
CMD npm run dev -- --host 0.0.0.0