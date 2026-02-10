#include <gtest/gtest.h>
#include "config.h"
#include <fstream>
#include <vector>

using namespace control_plane;

// Helper function to check if file exists
static bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Try to find config/config.yaml relative to common build locations
        // Tests typically run from build/ directory
        std::vector<std::string> possible_paths = {
#ifdef PROJECT_SOURCE_DIR
            std::string(PROJECT_SOURCE_DIR) + "/config/config.yaml",  // Absolute path
#endif
            "../../config/config.yaml",  // From build/
            "../config/config.yaml",      // Alternative
            "config/config.yaml"          // If run from root
        };
        
        config_path = "";
        for (const auto& path : possible_paths) {
            if (file_exists(path)) {
                config_path = path;
                break;
            }
        }
    }
    
    std::string config_path;
};

TEST_F(ConfigTest, LoadConfigFromFile) {
    // Skip test if config file doesn't exist
    if (config_path.empty() || !file_exists(config_path)) {
        GTEST_SKIP() << "Config file not found. Tried multiple paths.";
    }
    
    Config config = Config::load_from_file(config_path);
    
    // Assert expected values from config/config.yaml
    EXPECT_EQ(config.ports_count, 8) << "ports_count should be 8";
    EXPECT_EQ(config.tick_ms, 100) << "tick_ms should be 100";
    EXPECT_DOUBLE_EQ(config.flap_probability, 0.01) << "flap_probability should be 0.01";
    EXPECT_EQ(config.flap_min_ms, 500) << "flap_min_ms should be 500";
    EXPECT_EQ(config.flap_max_ms, 5000) << "flap_max_ms should be 5000";
    EXPECT_EQ(config.log_level, "info") << "log_level should be 'info'";
    EXPECT_EQ(config.http_port, 8080) << "http_port should be 8080";
}

TEST_F(ConfigTest, LoadConfigWithDefaults) {
    // Load a non-existent file - should use defaults
    Config config = Config::load_from_file("/nonexistent/config.yaml");
    
    EXPECT_EQ(config.ports_count, 8);
    EXPECT_EQ(config.tick_ms, 100);
    EXPECT_DOUBLE_EQ(config.flap_probability, 0.01);
    EXPECT_EQ(config.flap_min_ms, 500);
    EXPECT_EQ(config.flap_max_ms, 5000);
    EXPECT_EQ(config.log_level, "info");
    EXPECT_EQ(config.http_port, 8080);
}

TEST_F(ConfigTest, CLIOverridesYAML) {
    // Load config from file
    if (config_path.empty() || !file_exists(config_path)) {
        GTEST_SKIP() << "Config file not found. Tried multiple paths.";
    }
    
    Config config = Config::load_from_file(config_path);
    
    // Verify original values
    EXPECT_EQ(config.ports_count, 8);
    EXPECT_EQ(config.http_port, 8080);
    
    // Simulate CLI arguments
    const char* argv[] = {
        "test",
        "--ports", "16",
        "--http-port", "9090",
        "--tick-ms", "50"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    config.apply_cli_args(argc, const_cast<char**>(argv));
    
    // CLI should override YAML
    EXPECT_EQ(config.ports_count, 16);
    EXPECT_EQ(config.http_port, 9090);
    EXPECT_EQ(config.tick_ms, 50);
    
    // Other values should remain from YAML
    EXPECT_DOUBLE_EQ(config.flap_probability, 0.01);
    EXPECT_EQ(config.log_level, "info");
}

TEST_F(ConfigTest, ConfigValidation) {
    Config config;
    
    // Valid config should pass
    config.ports_count = 8;
    config.tick_ms = 100;
    config.flap_probability = 0.01;
    config.flap_min_ms = 500;
    config.flap_max_ms = 5000;
    config.http_port = 8080;
    EXPECT_TRUE(config.validate());
    
    // Invalid ports_count
    config.ports_count = 0;
    EXPECT_FALSE(config.validate());
    config.ports_count = 1001;
    EXPECT_FALSE(config.validate());
    
    // Reset to valid
    config.ports_count = 8;
    
    // Invalid tick_ms
    config.tick_ms = 0;
    EXPECT_FALSE(config.validate());
    config.tick_ms = 10001;
    EXPECT_FALSE(config.validate());
    
    // Reset to valid
    config.tick_ms = 100;
    
    // Invalid flap_probability
    config.flap_probability = -0.1;
    EXPECT_FALSE(config.validate());
    config.flap_probability = 1.1;
    EXPECT_FALSE(config.validate());
    
    // Reset to valid
    config.flap_probability = 0.01;
    
    // Invalid flap duration range
    config.flap_min_ms = 1000;
    config.flap_max_ms = 500;
    EXPECT_FALSE(config.validate());
    
    // Reset to valid
    config.flap_min_ms = 500;
    config.flap_max_ms = 5000;
    
    // Invalid http_port
    config.http_port = 0;
    EXPECT_FALSE(config.validate());
    config.http_port = 65536;
    EXPECT_FALSE(config.validate());
}

TEST_F(ConfigTest, ConfigToString) {
    Config config;
    config.ports_count = 4;
    config.tick_ms = 50;
    config.flap_probability = 0.05;
    config.flap_min_ms = 100;
    config.flap_max_ms = 2000;
    config.log_level = "debug";
    config.http_port = 9090;
    config.seed = 12345;
    
    std::string str = config.to_string();
    
    EXPECT_NE(str.find("ports_count: 4"), std::string::npos);
    EXPECT_NE(str.find("tick_ms: 50"), std::string::npos);
    EXPECT_NE(str.find("log_level: debug"), std::string::npos);
    EXPECT_NE(str.find("http_port: 9090"), std::string::npos);
    EXPECT_NE(str.find("seed: 12345"), std::string::npos);
}
