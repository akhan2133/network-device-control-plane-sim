pipeline {
    agent any
    
    environment {
        BUILD_DIR = 'build'
        BINARY_NAME = 'control_plane_sim'
    }
    
    stages {
        stage('Checkout') {
            steps {
                echo 'Checking out source code...'
                checkout scm
            }
        }
        
        stage('Build') {
            steps {
                echo 'Building Control Plane Simulator...'
                sh '''
                    mkdir -p ${BUILD_DIR}
                    cd ${BUILD_DIR}
                    cmake -DCMAKE_BUILD_TYPE=Release ..
                    make -j$(nproc)
                '''
            }
        }
        
        stage('Unit Tests') {
            steps {
                echo 'Running unit tests...'
                sh '''
                    cd ${BUILD_DIR}
                    ctest --output-on-failure --verbose
                '''
            }
            post {
                always {
                    // Archive test results if available
                    junit allowEmptyResults: true, testResults: '**/test-results/*.xml'
                }
            }
        }
        
        stage('Integration Test') {
            steps {
                echo 'Running integration tests...'
                sh '''
                    chmod +x scripts/integration_test.sh
                    ./scripts/integration_test.sh
                '''
            }
        }
        
        stage('Docker Build') {
            steps {
                echo 'Building Docker image...'
                sh '''
                    docker build -t control-plane-sim:${BUILD_NUMBER} .
                    docker tag control-plane-sim:${BUILD_NUMBER} control-plane-sim:latest
                '''
            }
        }
        
        stage('Archive Artifacts') {
            steps {
                echo 'Archiving build artifacts...'
                archiveArtifacts artifacts: 'build/bin/*', fingerprint: true
            }
        }
    }
    
    post {
        success {
            echo 'Pipeline completed successfully!'
        }
        failure {
            echo 'Pipeline failed. Check logs for details.'
        }
        always {
            echo 'Cleaning up workspace...'
            cleanWs()
        }
    }
}
