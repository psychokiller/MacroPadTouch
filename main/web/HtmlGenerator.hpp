#ifndef HTML_GENERATOR_HPP
#define HTML_GENERATOR_HPP

#include <string>
#include <sstream>
#include <algorithm>
#include "FileManager.hpp"
#include <vector>
#include "WifiManager.hpp"

// Utility function definition for use in HTML generation
inline std::string bytes_to_human_readable(size_t bytes)
{
    // ... (utility function remains the same)
    const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double d_bytes = (double)bytes;

    while (d_bytes >= 1024.0 && i < 4)
    {
        d_bytes /= 1024.0;
        i++;
    }

    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << d_bytes << " " << suffixes[i];
    return ss.str();
}

/**
 * @brief Namespace dedicated to generating all required HTML pages.
 * Responsibility: Presentation and UI structure (View Layer).
 */
namespace HtmlGenerator
{

    // Common CSS for all configuration pages
    const char *CONFIG_CSS = R"(
        body { font-family: 'Arial', sans-serif; background-color: #f4f7f6; color: #333; margin: 0; padding: 20px; }
        .container { max-width: 500px; margin: 0 auto; background: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        h1 { color: #007bff; border-bottom: 2px solid #e9ecef; padding-bottom: 10px; margin-top: 0; }
        label { display: block; margin-top: 15px; font-weight: bold; }
        input[type='text'], input[type='password'] { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
        input[type='submit'] { background-color: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; }
        input[type='submit']:hover { background-color: #0056b3; }
        .success { color: green; font-weight: bold; }
        .error { color: red; font-weight: bold; }
        .back-link { display: block; margin-top: 20px; color: #007bff; text-decoration: none; }
        .network-list { list-style: none; padding: 0; }
        .network-item { margin-bottom: 10px; padding: 15px; border: 1px solid #e9ecef; border-radius: 4px; background-color: #fff; } /* IMPROVED: Better item styling */
        .network-header { display: flex; justify-content: space-between; align-items: center; } /* IMPROVED: Header for SSID and button */
        .ssid-name { font-weight: bold; font-size: 1.1em; } /* IMPROVED: Highlight SSID */
        .connect-button { background-color: #28a745; color: white; padding: 8px 12px; border: none; border-radius: 4px; cursor: pointer; }
        .connect-button:hover { background-color: #218838; }
        .password-field { margin-top: 10px; display: none; } /* IMPROVED: Initially hide password field */
        .password-field input { margin-top: 5px; } /* IMPROVED: Better spacing */
    )";

    inline std::string generate_sta_config_page(const std::string &status_message)
    {
        std::stringstream html;
        html << R"(<!DOCTYPE html><html><head>
     <meta charset='UTF-8'><title>Configure STA</title>
     <meta name='viewport' content='width=device-width, initial-scale=1.0'>
     <style>)"
             << CONFIG_CSS << R"(</style>
     </head><body>
     <div class='container'>
     <h1>Wi-Fi Station Configuration</h1>
     )" << status_message
             << R"(
     <ul class='network-list'>)";

        // NOTE: The original code used a mock WifiManager and ESP_LOGI. I'm keeping the structure
        // but removing the ESP_LOGI calls as they are device-specific and not HTML.
        // Assuming WifiManager::getInstance() and get_networks() are available.
        // WifiManager& wifi_manager = WifiManager::getInstance();
        // std::vector<WifiNetwork> networks = wifi_manager.get_networks();
        // Mock data for demonstration purposes in an incomplete environment:

        WifiManager& wifi_manager = WifiManager::getInstance();
        std::vector<WifiNetwork> networks = wifi_manager.get_networks();
    

        for (const auto &network : networks)
        {
            html << "<li class='network-item'>";
            html << "<div class='network-header'>";
            html << "<span class='ssid-name'>" << network.ssid << (network.is_open ? " (Open)" : " (Secured)") << "</span>";

            // Button to trigger the connection logic/password input
            html << "<button class='connect-button' onclick=\"togglePasswordForm('" << network.ssid << "', " << (network.is_open ? "true" : "false") << ")\">Connect</button>";
            html << "</div>";

            // Form is now contained in a div, initially hidden for secured networks
            // The form action is dynamic to allow for direct connection or password submission
            html << "<div id='form-" << network.ssid << "' class='password-field'>";
            html << "<form method='POST' action='/wifi/sta' data-open='" << (network.is_open ? "true" : "false") << "'>";
            html << "<input type='hidden' name='ssid' value='" << network.ssid << "'>";

            if (!network.is_open)
            {
                html << "<label for='password-" << network.ssid << "'>Password:</label>";
                html << "<input type='password' id='password-" << network.ssid << "' name='password' required>";
            }

            html << "<input type='submit' value='Join' style='margin-top: 10px; width: auto;'>";
            html << "</form>";
            html << "</div>";

            html << "</li>";
        }

        html << R"(</ul>
     <a href='/' class='back-link'>&#9664; Back to File Manager</a>
     <p><small>Select a network to connect to.</small></p>
     </div>
     
     <script>
     // JavaScript to handle showing/hiding the password form
     function togglePasswordForm(ssid, isOpen) {
        var formDiv = document.getElementById('form-' + ssid);
        var forms = document.querySelectorAll('.password-field');

        // 1. Hide all other forms
        forms.forEach(function(div) {
            if (div.id !== 'form-' + ssid) {
                div.style.display = 'none';
            }
        });

        if (isOpen) {
            // 2. If open network, submit immediately by creating a temporary form
            var tempForm = document.createElement('form');
            tempForm.method = 'POST';
            tempForm.action = '/wifi/sta';
            var ssidInput = document.createElement('input');
            ssidInput.type = 'hidden';
            ssidInput.name = 'ssid';
            ssidInput.value = ssid;
            tempForm.appendChild(ssidInput);
            document.body.appendChild(tempForm);
            tempForm.submit();
        } else {
            // 3. If secured, toggle the password field
            if (formDiv.style.display === 'block') {
                formDiv.style.display = 'none';
            } else {
                formDiv.style.display = 'block';
            }
        }
     }
     </script>
     
     </body></html>)";
        return html.str();
    }

    inline std::string generate_ap_config_page(const char *current_ssid, const char *current_pass, const std::string &status_message)
    {
        std::stringstream html;
        html << R"(<!DOCTYPE html><html><head>
        <meta charset='UTF-8'><title>Configure AP</title>
        <meta name='viewport' content='width=device-width, initial-scale=1.0'>
        <style>)"
             << CONFIG_CSS << R"(</style>
        </head><body>
        <div class='container'>
        <h1>Access Point Configuration</h1>
        )" << status_message
             << R"(
        <form method='POST' action='/wifi/ap'>
            <label for='ssid'>Access Point SSID:</label>
            <input type='text' id='ssid' name='ssid' value=')"
             << current_ssid << R"(' required>

            <label for='password'>Password (min 8 chars):</label>
            <input type='password' id='password' name='password' value=')"
             << current_pass << R"(' required>
            
            <input type='submit' value='Save & Reboot'>
        </form>
        <a href='/' class='back-link'>&#9664; Back to File Manager</a>
        <p><small>Saving these credentials will cause the ESP32 to reboot and use these settings when in AP configuration mode.</small></p>
        </div>
        </body></html>)";
        return html.str();
    }

    inline std::string generate_file_list_table(FileManager *fm)
    {
        std::stringstream table_content;
        std::vector<std::string> files = fm->list_files();

        if (files.empty())
        {
            table_content << R"(
                <tr><td colspan="3">No files found on SPIFFS.</td></tr>
            )";
        }
        else
        {
            for (const std::string &filename : files)
            {
                FileManager::FileInfo info = fm->get_file_info(filename.c_str());
                std::string human_size = bytes_to_human_readable(info.size);

                table_content << "<tr>";

                // Filename and Download Link
                table_content << "<td><a href=\"/" << filename << "\">" << filename << "</a></td>";

                // Size
                table_content << "<td>" << human_size << "</td>";

                // Actions (Delete Form)
                table_content << "<td>"
                              << "<form class=\"action-form\" method=\"POST\" action=\"/delete/" << filename << "\" onsubmit=\"return confirm('Are you sure you want to delete " << filename << "?');\">"
                              << "<button type=\"submit\">Delete</button>"
                              << "</form>"
                              << "</td>";

                table_content << "</tr>";
            }
        }
        return table_content.str();
    }

    inline std::string generate_root_page(FileManager *fm)
    {
        std::stringstream html_content;

        // Start of HTML page
        html_content << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32 Device Manager</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: 'Arial', sans-serif; background-color: #f4f7f6; color: #333; margin: 0; padding: 20px; }
        .container { max-width: 800px; margin: 0 auto; background: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        h1, h2 { color: #007bff; border-bottom: 2px solid #e9ecef; padding-bottom: 10px; margin-top: 20px; }
        .menu-bar { margin-bottom: 30px; padding: 10px; background-color: #e9ecef; border-radius: 6px; }
        .menu-bar a { display: inline-block; padding: 8px 15px; background-color: #28a745; color: white; text-decoration: none; border-radius: 4px; margin-right: 10px; transition: background-color 0.3s; }
        .menu-bar a:hover { background-color: #1e7e34; }
        .file-list { width: 100%; border-collapse: collapse; margin-top: 20px; }
        .file-list th, .file-list td { padding: 12px 15px; text-align: left; border-bottom: 1px solid #ddd; }
        .file-list th { background-color: #007bff; color: white; }
        .file-list tr:hover { background-color: #f1f1f1; }
        .file-list a { color: #007bff; text-decoration: none; font-weight: bold; }
        .file-list a:hover { text-decoration: underline; }
        .upload-form input[type="file"], .upload-form input[type="submit"] {
            padding: 10px; margin: 5px 0; border: 1px solid #ccc; border-radius: 4px; 
            box-sizing: border-box; display: inline-block;
        }
        .upload-form input[type="submit"] { background-color: #28a745; color: white; cursor: pointer; border: none; }
        .upload-form input[type="submit"]:hover { background-color: #218838; }
        .action-form { display: inline; margin-left: 10px; }
        .action-form button { background: none; border: none; color: #dc3545; cursor: pointer; font-weight: bold; }
        .action-form button:hover { text-decoration: underline; }
    </style>
</head>
<body>
<div class="container">
    <h1>ESP32 Device Manager</h1>

    <div class="menu-bar">
        <a href="/wifi/sta" style="background-color: #ffc107;">Configure STA (Client)</a>
        <a href="/wifi/ap" style="background-color: #17a2b8;">Configure AP (Host)</a>
    </div>
    
    <h2>Upload File</h2>
    <form class="upload-form" action="/upload/filename" method="post" enctype="multipart/form-data" id="uploadForm">
        <input type="file" name="file" id="fileInput" required>
        <input type="submit" value="Upload">
        <p><small>Note: Filename is taken from the browser and appended to the <code>/upload/</code> path.</small></p>
    </form>
    
    <script>
        // Simple JavaScript to grab filename before submitting
        document.getElementById('uploadForm').onsubmit = function() {
            var file = document.getElementById('fileInput').files[0];
            if (file) {
                var newAction = '/upload/' + file.name;
                this.action = newAction;
            }
        };
    </script>

    <h2>Existing Files </h2>
    <table class="file-list">
        <thead>
            <tr>
                <th>Filename</th>
                <th>Size</th>
                <th>Actions</th>
            </tr>
        </thead>
        <tbody>
)" << generate_file_list_table(fm)
                     << R"(
        </tbody>
    </table>
</div>
</body>
</html>
)";

        return html_content.str();
    }
}

#endif // HTML_GENERATOR_HPP
