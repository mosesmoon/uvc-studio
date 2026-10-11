# Giới thiệu tính năng UVC Studio

[English](README.md) · [台灣正體中文](README.zh-TW.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Italiano](README.it.md) · [Español](README.es.md) · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · **Tiếng Việt**

UVC Studio 1.1.0 là ứng dụng gốc (native) dành cho card capture, chạy hoàn toàn ngoại tuyến trên Linux, Windows và Meta Quest: chơi máy console qua bất kỳ card capture UVC nào với độ trễ thấp, nâng độ phân giải lên 4K, thêm hiệu ứng retro hoặc chiều sâu 3D, đặt hình webcam lên trên game, ghi hình và chụp ảnh — mà không bao giờ kết nối mạng. 1.1.0, phiên bản chính thức đầu tiên, mang UVC Studio Platinum lên Meta Quest: nâng cấp 4K và chiều sâu 3D vẫn miễn phí 30 phút mỗi ngày cho mỗi tính năng, và mua một lần sẽ mở khóa vĩnh viễn; phiên bản Linux và Windows vẫn miễn phí và giờ là mã nguồn mở theo giấy phép MIT.

![UVC Studio khi chơi game với nâng cấp 4K](docs/screenshots/02-playing-4k.png)

## Nền tảng và tải về

Cùng một mã nguồn nên mọi nền tảng có tính năng và số phiên bản giống nhau.

| Nền tảng | Tải về | Thu hình | Âm thanh | Đồ họa | Ghi hình |
| --- | --- | --- | --- | --- | --- |
| Linux (x86_64) | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264 (phần cứng VA-API / NVENC / AMF) + AAC, MP4 — cần một trong số này |
| Windows (x64) | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe) (bản cài đặt) hoặc [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip) (bản portable) | Media Foundation | WASAPI | OpenGL 3.3 | H.264 (NVENC / AMF / Intel qua Media Foundation, nếu không thì dùng bộ mã hóa có sẵn của Windows) + AAC, MP4 — cần một trong số này |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk) (sideload) hoặc Meta Horizon Store (sắp ra mắt) | USB host, libuvc | Đọc trực tiếp âm thanh USB của card (stereo hoặc 5.1) | OpenGL ES 3 (cửa sổ 2D), OpenXR (nhập vai) | H.264 + AAC (MediaCodec phần cứng), MP4 |

Mã nguồn (Linux và Windows, giấy phép MIT): kho mã này, hoặc [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz).

Trên Meta Quest, nâng cấp 4K và chiều sâu 3D được miễn phí 30 phút mỗi ngày cho mỗi tính năng; UVC Studio Platinum, mua một lần trong ứng dụng, giúp dùng không giới hạn. Trên Linux và Windows, mọi tính năng đều miễn phí.

## Ảnh chụp màn hình

Linux ở 1920 × 1080; bấm vào ảnh để xem kích thước đầy đủ. Cửa sổ 2D trên Meta Quest dùng cùng giao diện.

<table>
<tr><td width="50%"><a href="docs/screenshots/01-ready.png"><img src="docs/screenshots/01-ready.png" alt="Màn hình chờ: kiểm tra trực tiếp card capture, âm thanh game và webcam"></a><br><sub>Màn hình chờ: kiểm tra trực tiếp card capture, âm thanh game và webcam</sub></td><td width="50%"><a href="docs/screenshots/03-4k-upscaling.png"><img src="docs/screenshots/03-4k-upscaling.png" alt="Nâng cấp 4K: bốn cách để chọn"></a><br><sub>Nâng cấp 4K: bốn cách để chọn</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/04-settings.png"><img src="docs/screenshots/04-settings.png" alt="Cài đặt: mọi thứ trên một bảng kính"></a><br><sub>Cài đặt: mọi thứ trên một bảng kính</sub></td><td width="50%"><a href="docs/screenshots/05-webcam-overlay.png"><img src="docs/screenshots/05-webcam-overlay.png" alt="Webcam đặt lên trên game"></a><br><sub>Webcam đặt lên trên game</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/06-retro-vhs.png"><img src="docs/screenshots/06-retro-vhs.png" alt="Hiệu ứng retro: VHS"></a><br><sub>Hiệu ứng retro: VHS</sub></td><td width="50%"><a href="docs/screenshots/07-3d-depth.png"><img src="docs/screenshots/07-3d-depth.png" alt="Chiều sâu 3D"></a><br><sub>Chiều sâu 3D</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/08-shortcuts.png"><img src="docs/screenshots/08-shortcuts.png" alt="Phím tắt"></a><br><sub>Phím tắt</sub></td><td width="50%"><a href="docs/screenshots/09-light-theme.png"><img src="docs/screenshots/09-light-theme.png" alt="Giao diện sáng"></a><br><sub>Giao diện sáng</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/10-zh-TW.png"><img src="docs/screenshots/10-zh-TW.png" alt="Giao diện 台灣正體中文"></a><br><sub>Giao diện 台灣正體中文</sub></td><td width="50%"><a href="docs/screenshots/11-languages.png"><img src="docs/screenshots/11-languages.png" alt="12 ngôn ngữ giao diện"></a><br><sub>12 ngôn ngữ giao diện</sub></td></tr>
</table>

## Bắt đầu

1. Cắm card capture vào cổng USB rồi mở UVC Studio. **Màn hình khởi đầu** kiểm tra trực tiếp ba mục: *Card capture*, *Âm thanh game* và *Webcam (tùy chọn)*. Nếu thiếu card sẽ hiện "Không tìm thấy — hãy cắm vào cổng USB"; nếu thiếu âm thanh sẽ hiện "Không tìm thấy (chỉ có hình)".
2. Nhấn **Kết nối** (trên màn hình khởi đầu hoặc ở góc trên bên phải) hoặc phím `Enter`. Khi **Tự động kết nối** được bật (Cài đặt → Chung, bật sẵn theo mặc định), UVC Studio tự kết nối ngay khi bạn cắm card, và cả lúc mở ứng dụng nếu card đã được cắm sẵn.
3. Chơi thôi. **Ngắt kết nối** (cùng nút đó, hoặc `Ctrl+D`) đóng mọi thứ; nếu đang ghi hình, ứng dụng sẽ dừng ghi và lưu tệp trước.

Không còn tạm dừng hay tiếp tục nữa: UVC Studio chỉ có hai trạng thái, đã kết nối hoặc chưa kết nối. Nếu card bị rút ra, thông báo lỗi sẽ hiện dưới trạng thái kết nối và ứng dụng quay về màn hình khởi đầu; khi bật tự động kết nối, ứng dụng sẽ kết nối lại ngay khi card được cắm vào.

## Thu hình và âm thanh

Mọi card capture UVC đều dùng được mà không cần cài thêm driver: ShadowCast, Elgato, AVerMedia, Magewell hay các dongle HDMI USB 3 phổ thông đều hoạt động như nhau.

- **Dùng đúng chế độ của card**: độ phân giải và tốc độ khung hình lấy từ danh sách card báo về, xếp từ lớn đến nhỏ kèm nhãn 4K, QHD, Full HD, HD hoặc SD. Mặc định, UVC Studio chọn độ phân giải lớn nhất đạt từ 59 fps trở lên. Định dạng: Tự động, Không nén (độ trễ thấp nhất) hoặc Nén (cho phép độ phân giải và tốc độ khung hình cao hơn).
- **Chọn đúng thiết bị**: khi chưa có lựa chọn nào được lưu, ứng dụng dựa vào tên thiết bị để chọn card capture, âm thanh của card và webcam, nên không nhầm camera tích hợp hay camera ảo thành card capture. Các thiết bị trùng tên được thêm (2), (3)… để phân biệt.
- **Độ trễ thấp**: luôn hiển thị khung hình mới nhất, không để khung hình cũ dồn lại.
- **Ghi nhớ thiết lập** và nhận ra ngay khi bạn cắm hoặc rút card trong lúc ứng dụng đang chạy.
- **Nghe** (`L`): nghe âm thanh game qua loa hoặc tai nghe. Rê chuột lên nút trên Dock sẽ thấy công tắc bật/tắt, âm lượng và thiết bị đầu ra hiện tại; dòng "Đầu ra" đưa bạn thẳng tới Cài đặt → Âm thanh.
- **Hiển thị chế độ thực tế**: Cài đặt → Nguồn cho biết luồng đang chạy, ví dụ 3840 × 2160 · 60 fps · YUY2.
- **Âm thanh trên Meta Quest**: ứng dụng đọc âm thanh của card trực tiếp qua USB (USB Audio Class 1 và 2, kể cả dongle MacroSilicon MS2109) — Cài đặt → Âm thanh → Đầu vào âm thanh → "Âm thanh card capture (USB)". Cài đặt → Âm thanh → "Kênh âm thanh game": **Stereo 2CH** (mặc định) hoặc **Vòm 6CH** — phát 5.1 với card có chế độ 6 kênh (như CJ-16) và máy console xuất 5.1 qua HDMI (như Switch 2 đặt ở Vòm); bản ghi vẫn là stereo.
- **Tự phục hồi trên Quest**: nếu card ngừng gửi hình, ứng dụng tự mở lại card (tối đa hai lần), sau đó nhắc bạn kiểm tra nguồn HDMI hoặc rút card ra cắm lại.

## Hình ảnh

Bốn cách nâng độ phân giải 4K, ba hiệu ứng retro và chiều sâu 3D đều chạy trên GPU theo từng khung hình, không thêm độ trễ nào ngoài thời gian GPU xử lý.

**Nâng độ phân giải 4K** (`U`): nhấp vào nút trên Dock hoặc nhấn `U` để bật/tắt, rê chuột lên nút để chọn cách, nhấn `Shift+U` để chuyển sang cách tiếp theo. Mục Cài đặt → Hình ảnh → Nâng độ phân giải 4K có *Tắt*, *FSR*, *Anime4K*, *Lanczos* và *NIS*: chọn một cách sẽ bật nâng độ phân giải, còn *Tắt* vẫn ghi nhớ cách dùng gần nhất cho lần sau.

| Cách | Nguyên lý | Phù hợp với | Hệ số |
| --- | --- | --- | --- |
| Nâng độ phân giải 4K bằng FSR (mặc định) | AMD FidelityFX Super Resolution 1: phóng to thích ứng theo cạnh (EASU) rồi làm nét bằng RCAS | Game và mọi loại video | 720p / 1080p lên 4K; nguồn SD tối đa 3× |
| Nâng độ phân giải 4K bằng Anime4K cho hoạt hình | Mạng nơ-ron Anime4K ×2 (bản M trên máy tính, bản S trên Quest), phần còn lại do FSR đảm nhận | Anime, phim hoạt hình, tranh nét | Mạng nơ-ron phóng 2×, sau đó lên 4K |
| Nâng độ phân giải 4K bằng Lanczos + CAS | Lấy mẫu lại Lanczos-3 (6 điểm mỗi trục) rồi làm nét bằng AMD FidelityFX CAS | Phóng to kiểu cổ điển, trung tính | Lên 4K |
| Nâng độ phân giải 4K bằng NIS | NVIDIA Image Scaling 1.0.3: phóng to 6 điểm theo hướng cạnh kèm làm nét thích ứng | Chi tiết sắc nét | Tối đa 2×, phần còn lại dùng Lanczos-3 |

Bộ nâng độ phân giải chỉ tính lại khi có khung hình mới. Nếu Quest không hỗ trợ render target dạng half-float, Anime4K sẽ chuyển sang FSR.

- **Hiệu ứng retro** (`C`): rê chuột lên nút trên Dock để chọn *Tắt*, *CRT*, *VHS* hoặc *Arcade*; nhấn `C` để chuyển lần lượt theo thứ tự đó, còn nhấp vào nút sẽ chuyển qua lại giữa *Tắt* và hiệu ứng dùng gần nhất. Mục Cài đặt → Hình ảnh → Hiệu ứng retro cũng có lựa chọn tương tự. Hiệu ứng được áp lên hình cuối cùng, sau bước nâng độ phân giải 4K, nên dùng được cùng lúc với 4K, và hiện cả trong ảnh chụp, video ghi lại lẫn trên màn hình khổng lồ của Quest. Lựa chọn của bạn được ghi nhớ.
  - **CRT**: TV ống — mặt nạ khe so le, đường quét đậm dần theo độ sáng, màn hình hơi cong và viền tối mềm mại.
  - **VHS**: băng video gia đình — hình hơi mềm, màu loang sang phải, tông màu ấm hơi bạc với màu đen hơi xám, nhiễu nhẹ, các dòng rung khẽ và thỉnh thoảng có một dải nhiễu tracking mờ trôi từ trên xuống (chữ vẫn đọc rõ).
  - **Arcade**: màn hình máy arcade — hình độ phân giải thấp với đường quét tối đậm, lưới RGB dọc, ánh sáng tỏa nhẹ quanh vùng sáng, màu rực rỡ và độ cong nhẹ.
- **Chiều sâu 3D** (`D`): ước lượng phần nào trong hình ở gần, phần nào ở xa. Rê chuột lên nút, hoặc mở Cài đặt → Chiều sâu 3D, để chọn **Định dạng chiều sâu** và **Cường độ hiệu ứng**. *Hiệu ứng chiều sâu (chỉ tăng cường 2D)* làm tiền cảnh sắc nét và rực rỡ hơn, hậu cảnh mềm và mờ ảo hơn; các định dạng lập thể cho màn hình 3D được mô tả ở phần dưới; ở chế độ nhập vai trên Quest, *Màn hình khổng lồ 3D lập thể* biến màn hình khổng lồ thành 3D lập thể thật sự. Nhấn `D` hoặc nhấp vào nút để chuyển qua lại giữa *Không có chiều sâu* và định dạng dùng gần nhất.
- **Độ sáng, tương phản, độ bão hòa** (Cài đặt → Hình ảnh): từ −40 đến +40, trong đó 0 là hình gốc. **Gamma**: từ 0,70 đến 1,40 (1,00 là hình gốc); trên 1,00 làm sáng các tông trung gian, dưới 1,00 làm chúng đậm hơn, còn màu đen và trắng giữ nguyên. Các **phong cách** có sẵn: *Tự nhiên*, *Chân thực* (độ tương phản tự nhiên theo đường cong chữ S nhẹ, màu sắc tiết chế, vẫn giữ chi tiết vùng tối), *Rực rỡ*, *Điện ảnh* và *Phòng sáng* (nay tăng gamma một chút để cảnh tối vẫn nhìn rõ trong phòng sáng).
- **Lật ngang** (`X`): lật toàn bộ hình từ trái sang phải.

## Định dạng 3D lập thể (Linux / Windows)

Trên máy tính, menu định dạng chiều sâu (ở nút Chiều sâu 3D hoặc trong Cài đặt → Chiều sâu 3D) còn có bốn định dạng lập thể để gửi hình 3D lập thể thật tới màn hình 3D. Đặt màn hình sang cùng định dạng 3D và chuyển sang toàn màn hình (`F`); ảnh chụp và video ghi lại vẫn là 2D. Chữ và các thành phần HUD được tự động nhận diện và giữ gần mặt phẳng màn hình, chiều sâu cũng mượt hơn, nên mỗi chữ dịch chuyển nguyên khối ở cả hai mắt, không còn trông như bị cắt đôi.

| Định dạng | Dành cho |
| --- | --- |
| Cạnh nhau, đầy đủ (3840×1080) | Kính XR ở chế độ 3D, như XREAL, VITURE và RayNeo |
| Cạnh nhau, một nửa | TV 3D |
| Trên-dưới | TV 3D và máy chiếu |
| Kính đỏ / xanh lơ | Kính anaglyph đỏ / xanh lơ |

## Lớp phủ webcam, chụp ảnh và ghi hình

Thấy gì lưu nấy: hiệu ứng, lật ngang và lớp phủ webcam đều có trong mọi ảnh chụp và video ghi lại.

- **Lớp phủ webcam** (`W`): đặt hình webcam lên trên game, ở một trong bốn góc (mặc định là góc dưới bên trái), với kích thước S, M hoặc L (18, 26 hoặc 36 % chiều rộng hình).
  - Kéo đến bất kỳ đâu; khi thả ra, lớp phủ tự trượt vào góc gần nhất. Lăn bánh xe chuột trên lớp phủ để đổi kích thước.
  - Rê chuột lên lớp phủ sẽ hiện thanh công cụ nhỏ với *Lật ngang*, *Kích thước* và *Gỡ bỏ*.
  - Tại nút trên Dock (hoặc Cài đặt → Lớp phủ webcam), bạn chọn *Camera*, *Chất lượng* (720p · 30 fps, 1080p · 30 fps hoặc 1080p · 60 fps), *Kích thước*, *Góc*, *Lật ngang* và **Đổi hình**: webcam trở thành hình chính còn card capture trở thành lớp phủ.
- **Chụp ảnh** (`S`): lưu hình hiện tại thành tệp PNG.
- **Ghi hình** (`R`): tệp MP4 với video H.264 và âm thanh AAC 192 kbit/s. Bitrate tính theo luồng (rộng × cao × fps × 0,12, trong khoảng 8–50 Mbit/s: khoảng 15 Mbit/s với 1080p60, 50 Mbit/s với 4K60). Khi đang ghi, trạng thái kết nối hiển thị REC và thời gian đã ghi.
  - **Windows:** không kèm bộ mã hóa phần mềm. Để ghi hình cần bộ mã hóa đồ họa của NVIDIA (NVENC), AMD (AMF) hoặc Intel, hoặc bộ mã hóa H.264 có sẵn trong Windows (Media Foundation; các bản Windows "N" cần cài Media Feature Pack).
  - **Linux:** cũng không kèm bộ mã hóa phần mềm. Để ghi hình cần đồ họa Intel hoặc AMD qua VA-API (đã cài driver VA-API), NVIDIA NVENC (driver độc quyền) hoặc AMD AMF.
  - Nếu không có cái nào, mọi tính năng khác vẫn dùng bình thường, chỉ có Ghi hình báo "Không có bộ mã hóa video: ghi hình cần đồ họa NVIDIA, AMD hoặc Intel".
- **Ghi âm micrô** (`M`): trộn giọng nói của bạn với âm thanh game.
- **Tên tệp và thư mục**: `UVC Studio 2026-10-06 21.30.15.png` / `.mp4` (ngày và giờ; nếu trùng tên sẽ thêm số). Ảnh chụp lưu vào **Pictures/UVC Studio**, video lưu vào **Videos/UVC Studio**, kèm thông báo "Đã lưu vào Pictures/UVC Studio" và nút **Xem**; trong Cài đặt → Chung còn có "Mở thư mục ảnh chụp" và "Mở thư mục video đã ghi". Trên Quest, tệp được lưu vào Thư viện (`Pictures/UVC Studio` và `Movies/UVC Studio`), xem được trong ứng dụng Tệp và qua USB.

## Meta Quest

Cắm card capture vào cổng USB-C của kính và chơi trong cửa sổ có thể đổi kích thước hoặc trên màn hình ảo khổng lồ, với 3D lập thể thật sự.

- **Cài đặt**: từ Meta Horizon Store (sắp ra mắt), hoặc bật chế độ nhà phát triển trên kính rồi cài `UVCStudio-Quest-v1.1.0-Free.apk` từ mục Releases của dự án này (Meta Quest Developer Hub, SideQuest hoặc `adb install`). Chỉ có thể mua Platinum trong phiên bản cài từ cửa hàng.
- **Kết nối**: dùng bộ chuyển USB-C OTG hoặc hub cũng được. Khi hệ thống hỏi có mở UVC Studio lúc cắm card không, hãy cho phép (chọn "Luôn luôn"), đồng thời cấp quyền camera và micrô mà Android yêu cầu đối với card capture USB. Ứng dụng không bao giờ dùng camera của chính chiếc kính.
- **Giải mã**: card gửi H.264 và HEVC được giải mã bằng bộ giải mã phần cứng tích hợp trong kính; FFmpeg đi kèm chỉ dùng cho MJPEG.
- **Cửa sổ 2D** (mặc định): toàn bộ giao diện giống bản máy tính trong một bảng Horizon OS có thể di chuyển và đổi kích thước. Chụm ngón tay hai lần lên hình để vào hoặc thoát toàn màn hình.
- **Chế độ nhập vai**: nút kính ở cuối Dock (sau Cài đặt) chuyển sang màn hình ảo khổng lồ và quay lại. Mỗi lần chuyển, UVC Studio tự ngắt kết nối rồi kết nối lại ở phía bên kia.
- **Chiều sâu 3D trên Quest**: trong cửa sổ 2D có thể chọn *Không có chiều sâu* hoặc *Hiệu ứng chiều sâu*; ở chế độ nhập vai là *Không có chiều sâu* hoặc *Màn hình khổng lồ 3D lập thể*.

| Trong chế độ nhập vai | Cách làm |
| --- | --- |
| Nhấp vào bảng điều khiển | Chỉ bằng tay cầm và bóp cò, hoặc chụm ngón tay |
| Di chuyển bảng điều khiển | Nắm thanh trắng bên dưới bảng và kéo; đẩy ra hoặc kéo vào để đổi khoảng cách. Cài đặt → VR → "Đặt lại vị trí bảng" để đưa về chỗ cũ |
| Di chuyển màn hình | Nắm thanh trắng bên dưới màn hình và kéo; đẩy tay ra hoặc kéo vào để đổi khoảng cách |
| Đổi kích thước màn hình | Chỉ vào một góc đến khi hiện khung góc, rồi chụm ngón tay và kéo |
| Gần hơn / xa hơn, nhỏ hơn / lớn hơn | Cần analog lên / xuống, trái / phải |
| Ẩn hoặc hiện bảng điều khiển | Chụm ngón tay hai lần lên màn hình, hoặc nhấn B, Y hay Menu |
| Căn giữa lại | Nhấn giữ nút Meta |
| Thiết lập sẵn cho màn hình | Cài đặt → VR: nền (Passthrough hoặc phòng tối), hình dạng màn hình (phẳng hoặc cong), kích thước màn hình, khoảng cách màn hình, tần số quét 72 / 90 / 120 Hz (mặc định 90) |

- **Menu hệ thống**: khi menu đang mở, ứng dụng vẫn tiếp tục vẽ hình nhưng ẩn con trỏ và tắt tiếng.
- **Giảm nhiệt**: chỉ nâng độ phân giải đến mức màn hình khổng lồ thực sự hiển thị được (khoảng 30 điểm ảnh mỗi độ, rộng tối đa 2560), và hình cho hai mắt cũng được dựng ở cùng kích thước đó.
- **Chống quá nhiệt**: theo dõi trạng thái nhiệt của Android và thông báo nhiệt của OpenXR. Khi kính bắt đầu nóng, ứng dụng chuyển sang FSR và 72 Hz; khi quá nóng, ứng dụng tạm dừng nâng độ phân giải và chiều sâu 3D. Cài đặt của bạn không bị thay đổi, và sẽ có thông báo khi kính đã nguội.
- **Hiển thị mức tải**: khi đang kết nối, Cài đặt → Giới thiệu cho biết thời gian GPU mỗi khung hình, độ phân giải xử lý và nhiệt độ kính VR.

## Giao diện

Toàn bộ giao diện theo phong cách Liquid Glass: các bề mặt mờ như kính nhám, trong lúc kết nối sẽ để lộ phía sau một bản sao của hình game được làm mờ và tăng độ rực màu.

- **Thanh trên cùng**: bên trái là logo UVC Studio; ở giữa là **trạng thái kết nối** ("Chưa kết nối"; chấm xanh lá kèm thiết bị · độ phân giải · fps, thêm ▸ 4K và tên bộ nâng độ phân giải, thêm · 3D khi bật chiều sâu 3D; hoặc REC đỏ nhấp nháy kèm thời gian ghi); bên phải là menu ngôn ngữ và nút **Kết nối / Ngắt kết nối**. Lỗi hiện bằng chữ đỏ dưới trạng thái trong vài giây.
- **Dock**: bốn nhóm — *Chụp ảnh, Ghi hình, Ghi âm micrô* · *Nâng độ phân giải 4K, Hiệu ứng retro, Chiều sâu 3D, Lật ngang* · *Lớp phủ webcam, Nghe* · *Cài đặt*. Nút dưới con trỏ sẽ phóng to, tính năng đang bật sáng lên bằng dải màu chuyển của thương hiệu, và mọi chú thích đều kèm phím tắt. Phiên bản nằm ở góc dưới bên trái, nút toàn màn hình ở góc dưới bên phải.
- **Cài đặt**: một tấm kính lớn với các thẻ xếp thành tối đa ba cột cân đối, chỉnh được mọi thứ mà không cần cuộn: Nguồn, Âm thanh, Hình ảnh, Lớp phủ webcam, Hiển thị, VR (chế độ nhập vai trên Quest), Chung, Chiều sâu 3D và Giới thiệu — trong bố cục tự động, thẻ "Chiều sâu 3D" nằm ngay phía trên "Giới thiệu". Thẻ "Giới thiệu" hiện phiên bản, thời gian GPU mỗi khung hình và độ phân giải xử lý khi đang kết nối (kèm nhiệt độ kính VR trên Quest) cùng nút "Giấy phép mã nguồn mở"; ghi chú về quyền riêng tư nằm trong trang mở ra từ nút này.
  - **Thu gọn thẻ**: nút tròn hình mũi tên ở tận cùng bên phải tiêu đề thẻ, bên phải ổ khóa, thu thẻ lại chỉ còn tiêu đề (tên, ổ khóa và nút thu gọn vẫn còn) và mở lại khi bấm lần nữa. Các thẻ bên dưới trượt lên, không thẻ nào đổi cột, và trạng thái thu gọn được ghi nhớ.
  - **Tự sắp xếp các thẻ**: mỗi thẻ có một ổ khóa ở góc trên bên phải phần tiêu đề, và ban đầu mọi thẻ đều bị khóa. Nhấp vào ổ khóa ("Mở khóa để di chuyển"), rồi kéo thẻ bằng phần tiêu đề đến vị trí khác trong ba cột; một ô trống cho biết thẻ sẽ được đặt ở đâu, và `Esc` hủy thao tác kéo. Nhấp vào ổ khóa lần nữa ("Khóa vị trí") để cố định thẻ. Các thẻ đang khóa giữ nguyên chỗ, còn thẻ đã mở khóa di chuyển xen giữa chúng. Cách sắp xếp và các thẻ đã mở khóa được ghi nhớ cho lần mở sau. Nút "Đặt lại bố cục" cạnh nút đóng, chỉ hiện khi bạn đang dùng cách sắp xếp riêng, sẽ khôi phục bố cục tự động và khóa lại mọi thẻ. Khi cửa sổ quá hẹp để hiện ba cột, bố cục tự động được hiển thị và cách sắp xếp của bạn vẫn được giữ cho tới khi cửa sổ đủ rộng trở lại. Trên Quest, cửa sổ 2D cũng thao tác y hệt bằng con trỏ của tay cầm.
- **Ngôn ngữ**: 台灣正體中文, English, Deutsch, Français, Italiano, Español, Português, 日本語, 한국어, 简体中文, ไทย và Tiếng Việt, chọn trong menu kính ở góc trên bên phải. Lần đầu mở, ứng dụng dùng ngôn ngữ của hệ thống.
- **Chủ đề**: Tự động (sáng từ 06:00 đến 18:00), Tối hoặc Sáng.
- **Cỡ giao diện**: tự điều chỉnh theo màn hình (1080p 100%, 1440p khoảng 135%, 4K 200%); có thể đổi trong Cài đặt → Hiển thị hoặc bằng `Ctrl` `+` / `−` / `0`.
- **Phông chữ và biểu tượng**: Manrope và IBM Plex Sans (IBM Plex Sans Thai cho tiếng Thái, Noto Sans CJK TC cho tiếng Trung, Nhật và Hàn); các biểu tượng do chính UVC Studio thiết kế.
- **Toàn màn hình**: `F`, `F11`, nút ở góc dưới bên phải hoặc nhấp đúp vào hình; `Esc` để thoát.

## Phím tắt (Linux / Windows)

Nhấn `?` hoặc `H` trong ứng dụng để xem đúng danh sách này; nhấn phím bất kỳ hoặc nhấp để đóng.

| Phím | Chức năng |
| --- | --- |
| `Enter` | Kết nối |
| `Ctrl+D` | Ngắt kết nối |
| `S` | Chụp ảnh |
| `R` | Bắt đầu hoặc dừng ghi hình |
| `M` | Bật / tắt ghi âm micrô |
| `L` | Bật / tắt Nghe |
| `W` | Bật / tắt lớp phủ webcam |
| `X` | Lật ngang |
| `U` | Bật / tắt nâng độ phân giải 4K |
| `Shift+U` | Cách nâng độ phân giải tiếp theo (đồng thời bật nâng độ phân giải) |
| `C` | Hiệu ứng retro: Tắt → CRT → VHS → Arcade |
| `D` | Chiều sâu 3D: chuyển giữa *Không có chiều sâu* và định dạng gần nhất |
| `O` hoặc `Ctrl+,` | Cài đặt |
| `F` hoặc `F11` | Toàn màn hình |
| `Esc` | Hủy kéo thẻ, thoát toàn màn hình hoặc đóng Cài đặt |
| `?` hoặc `H` | Phím tắt |
| `Ctrl` `+` / `−` / `0` | Phóng to / thu nhỏ / đặt lại giao diện |

## Quyền riêng tư

UVC Studio hoạt động hoàn toàn trên thiết bị của bạn. Hình ảnh, âm thanh và bản ghi không bao giờ rời khỏi thiết bị — không tài khoản, không tải lên, không theo dõi, và ứng dụng không bao giờ kết nối mạng. Trên Meta Quest, việc mua Platinum và kiểm tra giao dịch được thực hiện qua dịch vụ Meta Horizon Store của kính; bản thân UVC Studio không gửi gì cả. Chính sách quyền riêng tư đầy đủ có trong [PRIVACY.md](PRIVACY.md).

## Giấy phép

UVC Studio cho Linux và Windows là mã nguồn mở theo [giấy phép MIT](LICENSE); phiên bản Meta Quest không phải mã nguồn mở. UVC Studio không chứa thành phần GPL nào; FFmpeg được biên dịch chỉ với LGPL (trên Quest chỉ có bộ giải mã MJPEG, dưới dạng thư viện dùng chung có thể thay thế). Các thành phần khác: SDL2, Dear ImGui, miniaudio, stb, NanoSVG, libusb và libuvc (Quest), trình tải OpenXR, AMD FidelityFX FSR 1 và CAS, Anime4K và NVIDIA Image Scaling. Các phông chữ (Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC) dùng giấy phép SIL Open Font License. Danh sách đầy đủ các thành phần cùng toàn văn giấy phép có trong ứng dụng, tại Cài đặt → Giới thiệu → Giấy phép mã nguồn mở. Phiên bản Meta Quest còn kèm trình nạp Meta Platform SDK (Oculus SDK License) để mua trong cửa hàng.
