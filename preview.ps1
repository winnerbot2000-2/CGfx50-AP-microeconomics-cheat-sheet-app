$ErrorActionPreference = "Stop"

python tools\extract_apmicro_content.py
python tools\validate_content.py
python tools\start_preview.py
