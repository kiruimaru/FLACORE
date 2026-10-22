# GIT_GUIDE — как заливать FLACORE на GitHub

Практическая инструкция под состояние этого репозитория
(`https://github.com/kiruimaru/FLACORE`, ветка `main`, всё по HTTPS).
Все команды выполняются из корня проекта:

```bash
cd "/d/Documents GLM/FLACORE/FLACORE"
```

Порядок разделов = порядок действий. Если спешишь — внизу есть
[чек-лист](#чек-лист-всё-залито) и [шпаргалка](#шпаргалка).

---

## 0. Что сейчас в репозитории

Проверь сам, не верь на слово:

```bash
git status -sb
git log --oneline -5
git remote -v
```

Что важно на текущий момент:

- `main` синхронизирован с `origin/main` (пуш прошёл fast-forward);
- фича yt-dlp и двуязычный README **уже в `origin/main`** (коммиты
  `4a36327`, `d3ff602`, `bc9a7ab`), `GIT_GUIDE.md` сознательно не пушился;
- **`build/` всё ещё закоммичен** — 63 файла, включая `CMakeCache.txt` с личным
  путём `/home/kiruimaru/Projects/FLACORE` и Linux-бинарник `flacore` (ELF).
  Это стоит убрать из индекса (раздел 2);
- ветка `develop` отстала от `main` на 4 коммита и больше не используется;
- `gh` (GitHub CLI) не установлен, remote — HTTPS, учётные данные уже сохранены
  в Windows Credential Manager (credential helper `manager`), пуш работает без
  ввода пароля.

---

## 1. Кто ты для git

GitHub привязывает коммиты к аккаунту по e-mail. Сейчас локально стоит:

```bash
git config user.name      # nex1door
git config user.email     # hahdan14@gmail.com
```

Проверено: этот e-mail подтверждён на аккаунте `nex1door` (id 182537146), поэтому
коммиты с ним GitHub показывает как твои (`author.login = nex1door`, а не `null`).
Проверить для любого коммита можно через API:

```bash
curl -s "https://api.github.com/repos/kiruimaru/FLACORE/commits?per_page=3" \
  | python -c "import json,sys; [print(c['sha'][:7], (c.get('author') or {}).get('login')) for c in json.load(sys.stdin)]"
```

Если увидишь `None` — e-mail коммита не привязан к аккаунту, коммит останется
«безымянным» (без аватара и без записи в профиле).

Поставить другое имя/адрес:

```bash
git config --global user.name  "nex1door"
# вариант 1: свой подтверждённый e-mail
git config --global user.email "hahdan14@gmail.com"
# вариант 2: noreply-адрес — если не хочешь светить личную почту в коммитах
git config --global user.email "182537146+nex1door@users.noreply.github.com"
```

Noreply-адрес (вида `<id>+<login>@users.noreply.github.com`) берётся из
GitHub → Settings → Emails → Keep my email addresses private, либо из API:
`curl -s https://api.github.com/users/nex1door | grep '"id"'`.

> Права на пуш. Репозиторий принадлежит `kiruimaru`. Пушить в него напрямую
> можно только если тебя добавили в Settings → Collaborators. Иначе — fork +
> Pull Request (раздел 7). На этой машине пуш уже работает: учётные данные
> сохранены в Windows Credential Manager (credential helper `manager`).

---

## 2. Убрать `build/` из индекса

`.gitignore` содержит `build/`, но правило не действует на файлы, которые уже
отслеживаются. Убираем их **из индекса, оставляя на диске**:

```bash
git rm -r --cached build
git status --short        # увидишь кучу "D  build/..."
```

Файлы на диске останутся: `--cached` трогает только индекс. Проверка:

```bash
ls build/src/flacore.exe 2>/dev/null || ls build/src/flacore   # файл на месте
git check-ignore -v build/CMakeCache.txt                       # .gitignore:build/
```

Коммит (отдельный, чтобы история была читаемой):

```bash
git commit -m "chore(git): stop tracking build/ artifacts"
```

Заодно проверь, что в индексе нет мусора:

```bash
git ls-files | grep -E "^build/"            # пусто — хорошо
git ls-files | wc -l                        # сколько файлов реально в репо
```

> Учти: этот коммит **удалит** 63 файла из репозитория на GitHub (локально они
> останутся). Это правильное действие — в репозитории не место артефактам сборки
> с личными путями, — но делать его стоит осознанно, а не «за компанию».

---

## 3. Проверить правила игнорирования

Была реальная проблема: правило `core` в `.gitignore` игнорировало **всю**
папку `src/core/**`, поэтому новые файлы ядра не попадали в git. Уже исправлено
на `/core`. Проверить любой файл:

```bash
git check-ignore -v src/core/ytdlp/YtDlp.cpp   # пусто = файл НЕ игнорируется
git status --ignored --short | head -20        # что вообще игнорируется
```

Если файл неожиданно «пропал» из `git status` — почти всегда виноват
`.gitignore`, а не git.

---

## 4. Что именно коммитим

Список изменений под задачу «yt-dlp + документация»:

```bash
# новые файлы
git add src/core/ytdlp
git add src/ui/YouTubePanel.cpp src/ui/YouTubePanel.h
git add src/ui/YtDlpSettingsDialog.cpp src/ui/YtDlpSettingsDialog.h
git add tests/
git add GIT_GUIDE.md          # если решишь пушить и его

# изменённые файлы
git add .gitignore CMakeLists.txt src/CMakeLists.txt
git add src/ui/MainWindow.cpp src/ui/MainWindow.h
git add README.md
```

`git add -A` тоже сработает (после шага 2 `build/` уже игнорируется), но
адресный `git add` безопаснее: не затянет случайные файлы вроде `out.mp3`
или локальных заметок.

Проверить, что именно попало в индекс, **до** коммита:

```bash
git diff --cached --name-only
```

Если видишь лишнее — `git restore --staged <файл>`.

---

## 5. Проверка перед коммитом

Три команды, которые ловят 90 % проблем:

```bash
git status --short                # что попадёт в коммит
git diff --cached --stat          # размер изменений по файлам
git diff --check                  # пробелы в конце строк, конфликтные маркеры
```

Просмотр самого диффа (постранично, `q` — выход):

```bash
git diff --cached
```

Убедись, что в списке **нет**:

- `build/`, `*.exe`, `*.dll`, `*.so`, `*.a`, `*.obj`;
- музыки и скачанных треков (`*.flac`, `*.mp3`, `*.m4a`), если это не тестовые
  фикстуры;
- `CMakeCache.txt`, `compile_commands.json`, `.cache/`, `.qt/`;
- токенов, паролей, `.env`, `config/local_*.json`.

Проверить размер объектов в репозитории:

```bash
git count-objects -vH | grep -E "size-pack|count"
```

Нормально для этого проекта — единицы мегабайт. Если десятки — почти наверняка
опять артефакты сборки.

---

## 6. Коммиты

Стиль — [Conventional Commits](https://www.conventionalcommits.org/):
`type(scope): короткое описание в императиве`, тело — зачем, а не что.

Типы: `feat`, `fix`, `docs`, `chore`, `refactor`, `test`, `build`, `perf`.

Пример из этого репозитория — три логичных коммита вместо одного большого:

```bash
# 1) гигиена индекса
git add .gitignore
git commit -m "chore(git): fix 'core' ignore rule swallowing src/core"

# 2) фича
git add src CMakeLists.txt tests
git commit -m "feat(ytdlp): search, stream cache and FLAC export via yt-dlp" \
             -m "YtDlp wraps the CLI with QProcess: ytsearch/scsearch, direct-URL
metadata, progress via --progress-template, final path via
--print after_move:filepath, cancel by killing the child process.

YouTubePanel does not touch the player: it emits playRequested/fileAdded and
MainWindow decides what to do with the downloaded file.

FFmpeg libraries are no longer a build dependency (libav* were unused); the
ffmpeg binary is only needed for FLAC/MP3/Opus export.

Adds tests/fake_ytdlp.cpp and 39 assertions that run without network."

# 3) документация
git add README.md
git commit -m "docs(readme): bilingual README (English + Russian)"
```

Правила, которые экономят время:

- один коммит = одна мысль; «и ещё поправил вот это» — отдельным коммитом;
- сообщение в императиве: `add`, `fix`, `remove`, а не `added`/`fixes`;
- тело коммита объясняет **почему**, код объясняет **что**;
- не коммить сломанную сборку в `main`: сначала `cmake --build build`, `ctest`.

Посмотреть, что получилось:

```bash
git log --oneline --graph --decorate -10
git show --stat HEAD          # что вошло в последний коммит
```

Если заметил, что в коммит затянуло лишний файл, а он ещё не запушен — разбей
его правильно:

```bash
git reset --mixed HEAD~1      # откатить коммит, файлы остаются в рабочей копии
```

---

## 7. Пуш на GitHub

### Вариант A — напрямую в `main` (быстро, для соло-проекта)

```bash
git pull --rebase origin main     # подтянуть чужие изменения, если были
git push origin main
```

Ожидаемый вывод: `1586af5..bc9a7ab  main -> main`.

### Вариант B — через ветку и Pull Request (правильно для команды)

```bash
git switch -c feature/ytdlp-integration
git push -u origin feature/ytdlp-integration
```

Дальше на GitHub: **Compare & pull request** → описание → `Create pull request`.
После ревью — `Squash and merge`, затем локально:

```bash
git switch main
git pull origin main
git branch -d feature/ytdlp-integration
```

Если ветку слили squash-ем, `git branch -d` может ругнуться
(`not fully merged`) — тогда `git branch -D feature/ytdlp-integration`
(локальная ветка уже не нужна, код в `main`).

### Проверка, что пуш дошёл

```bash
git fetch origin
git log --oneline origin/main -4
git ls-tree -r --name-only origin/main | grep ytdlp
```

### Аутентификация

Пароль от GitHub по HTTPS **не работает** с 2021 года.

На этой машине уже сохранён рабочий credential helper (`manager`), поэтому
`git push` проходит без пароля. Если понадобится залогиниться заново:

**Personal Access Token:**

1. GitHub → Settings → Developer settings → Personal access tokens →
   **Fine-grained tokens** → Generate new token.
2. Repository access: только `FLACORE`. Permissions: **Contents: Read and write**.
3. Скопируй токен (показывается один раз) и вставь его вместо пароля, когда
   git спросит `Password for 'https://github.com'`.
4. Windows Credential Manager запомнит его сам; на Linux можно
   `git config --global credential.helper store` (токен ляжет в
   `~/.git-credentials` открытым текстом — только на личной машине).

Токен в URL (`https://<token>@github.com/...`) работать будет, но он попадёт в
`.git/config` и в историю команд — так лучше не делать.

**SSH (если планируешь часто пушить):**

```bash
ssh-keygen -t ed25519 -C "nex1door@github"
# Enter, Enter, passphrase по желанию
cat ~/.ssh/id_ed25519.pub      # скопировать в GitHub → Settings → SSH keys
ssh -T git@github.com          # ожидаемо: "Hi nex1door! You've successfully authenticated"
git remote set-url origin git@github.com:kiruimaru/FLACORE.git
```

**GitHub CLI** (`gh`) на этой машине не установлен. Если поставишь
(`winget install GitHub.cli`), дальше всё в одну команду:

```bash
gh auth login          # HTTPS + браузер, токен создастся сам
gh repo view --web
```

---

## 8. Ветки: что делать с `develop`

Сейчас `main` ушёл вперёд на 4 коммита, `develop` отстал,
`feature/phase-0-foundation` уже влит. Два разумных варианта:

**A. Оставить только trunk-based (рекомендую для 2 человек):**

```bash
git push origin --delete develop        # удалить на сервере
git branch -d develop 2>/dev/null       # и локально, если есть
```

Работать так: `main` всегда рабочий → фича в ветке `feature/<тема>` → PR → merge.

**B. Оживить `develop`:** тогда `main` — только релизы, а `develop` — интеграция:

```bash
git switch develop
git merge --ff-only main        # подтянуть отставшие коммиты
git push origin develop
```

Смешивать обе модели нельзя — получится ровно то, что сейчас: две «главные»
ветки, одна из которых мертва.

Именование веток: `feature/ytdlp-integration`, `fix/playlist-index-shift`,
`docs/readme`. Без пробелов и заглавных букв.

---

## 9. Теги и релизы

Версия живёт в `CMakeLists.txt` (`project(FLACORE VERSION 0.3.0 ...)`), поэтому
тег должен совпадать:

```bash
git switch main && git pull
git tag -a v0.3.0 -m "FLACORE 0.3.0: yt-dlp integration"
git push origin v0.3.0
```

На GitHub: **Releases → Draft a new release** → выбрать тег `v0.3.0` →
описание (что нового) → при желании приложить собранный бинарник.
Тег — это снимок кода; релиз — страница с описанием и файлами.

---

## 10. Если что-то пошло не так

**Закоммитил лишнее, но ещё не пушил:**

```bash
git reset --soft HEAD~1        # откатить коммит, изменения остаются в индексе
git reset --mixed HEAD~1       # откатить коммит и снять индексацию
git restore --staged <файл>    # убрать файл из индекса
```

**Опечатка в сообщении последнего коммита:**

```bash
git commit --amend -m "feat(ytdlp): search and download"
```

**Уже запушил и надо отменить:**

```bash
git revert <sha>               # безопасно: новый коммит, отменяющий старый
git push origin main
```

`git push --force` в `main` не использовать: перепишет историю у всех, кто уже
склонировал.

**Случайно закоммитил токен/пароль:** отменить коммит мало — файл остаётся в
истории. Меняй токен (revoke) и чисти историю через `git filter-repo`
(`git filter-repo --path secrets.txt --invert-paths`), потом force-push и
предупреди соавторов.

---

## 11. Частые ошибки и лечение

**`! [rejected] main -> main (fetch first)`**
Кто-то успел запушить. Не делай force:

```bash
git pull --rebase origin main
git push origin main
```

**`refusing to merge unrelated histories`**
Локальный и удалённый репозитории не имеют общего предка (например, репо
пересоздали):

```bash
git pull origin main --allow-unrelated-histories
```

**`fatal: refusing to add embedded repository` / странный пустой каталог**
Внутри проекта лежит чужой `.git` (например, склонированная зависимость).
Убирай вложенный `.git` или добавляй как submodule.

**Файл не появляется в `git status`**
`git check-ignore -v <файл>` — покажет правило `.gitignore`, которое его съело.

**Огромный дифф из-за переводов строк**
Windows проверяет CRLF, Linux — LF. Один раз нормализуй.

Создать `.gitattributes` в корне:

```
* text=auto
*.cpp text
*.h   text
*.md  text
*.txt text
*.png binary
*.ico binary
*.flac binary
*.mp3  binary
```

Затем:

```bash
git add --renormalize .
git commit -m "chore: normalize line endings"
```

Отдельным коммитом, чтобы не мешалось с содержательными правками.

**`Filename too long` на Windows**

```bash
git config --global core.longpaths true
```

**Два человека правят один файл и получают конфликт**
Конфликт решается вручную: в файле остаются маркеры `<<<<<<<`, `=======`,
`>>>>>>>`. Убираешь маркеры, оставляешь нужный код, затем
`git add <файл> && git commit`.

**Большие бинарники (треки, обложки, архивы)**
Больше ~50 МБ в git класть нельзя. Для картинок это не проблема, для аудио —
Git LFS:

```bash
git lfs install
git lfs track "*.flac" "*.mp3"
git add .gitattributes
```

Но правильнее — не хранить музыку в репозитории вообще.

---

## 12. Шпаргалка

```bash
git status -sb                 # состояние + расхождение с origin
git switch -c feature/x        # новая ветка от текущей
git add -p                     # добавлять по кускам (удобно перед коммитом)
git commit -m "type(scope): …" # коммит
git pull --rebase origin main  # подтянуть без merge-коммита
git push origin main           # отправить
git push -u origin feature/x   # первый пуш новой ветки
git log --oneline --graph -15  # история
git show --stat HEAD           # что в последнем коммите
git diff --cached --stat       # что в индексе
git restore <файл>             # отменить правки в файле
git restore --staged <файл>    # убрать из индекса
git stash / git stash pop      # отложить текущие правки
git remote -v                  # куда пушим
git branch -vv                 # ветки и их upstream
git tag -a v0.3.0 -m "…"       # релизный тег
git push origin --tags         # отправить теги
```

---

## 13. Чек-лист «всё залито»

```
[ ] git config user.name / user.email совпадают с аккаунтом GitHub
[ ] git rm -r --cached build  → коммит (артефакты больше не в репо)
[ ] git check-ignore -v src/core/ytdlp/YtDlp.cpp → пусто
[ ] git status --short → только осмысленные файлы
[ ] git diff --cached --check → без ошибок
[ ] сборка проходит: cmake --build build
[ ] тесты проходят:   ctest --test-dir build --output-on-failure
[ ] git log --oneline → читаемые коммиты, без «wip» и «fix»
[ ] git push origin main → успешно, ветка в синхроне
[ ] git fetch && git log --oneline origin/main -4 → коммиты на месте
[ ] на GitHub файлы видны, README отрендерился, бинарников в дереве нет
[ ] (по желанию) тег v0.3.0 и GitHub Release
```

После пуша проверь на сайте: `README.md` отображается с баннером, бейджами и
переключателем English/Русский, `src/core/ytdlp/` на месте, `tests/` присутствует.