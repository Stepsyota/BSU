# UML в PlantUML (лабораторная 8 — лифты)

| Файл | Диаграмма |
|------|-----------|
| `use-case.puml` | Прецедентов целиком (мало линий от акторов) |
| `use-case-operator.puml` | Только оператор — **для отчёта, рендер ровнее** |
| `use-case-passenger.puml` | Только пассажир |
| `activity.puml` | Активностей (всё на одной: сеанс, поездка, пожарка) |

## Как получить картинку или PDF

### Вариант A — расширение в CLion / VS Code / Cursor

1. Установить плагин **PlantUML integration** (в CLion: Settings → Plugins).
2. Убедиться, что установлен **Graphviz**: `brew install graphviz`.
3. Открыть `.puml` → предпросмотр / Export.

### Вариант B — jar в терминале (без аккаунтов)

```bash
brew install graphviz
brew install plantuml
cd /Users/home/BSU/TechProg/Lab/docs/uml
# PDF с кириллицей — только через SVG (прямой -tpdf для use case ломает текст):
plantuml -charset UTF-8 -tsvg -o pdf use-case-operator.puml use-case-passenger.puml activity.puml
for f in pdf/*.svg; do rsvg-convert -f pdf -o "${f%.svg}.pdf" "$f"; done
pdfunite pdf/use-case-elevators-operator.pdf pdf/use-case-elevators-passenger.pdf pdf/activity-elevators.pdf pdf/laba8-diagrammy.pdf
```

### CLion не меняет вид?

Плагин часто тянет **старый PlantUML** — `!pragma layout smetana` **не работает**. Соберите **jar с сайта** или Docker (один раз):

```bash
docker run --rm -v "$PWD:/work" -w /work plantuml/plantuml:latest -tsvg use-case-operator.puml
```

В отчёт: **рис. 1а + 1б** (`operator` + `passenger`) вместо одного забитого листа.

Появятся `use-case-elevators.png` и `activity-elevators.png` (имена из `@startuml`).

### Вариант C — онлайн

Скопировать содержимое файла на [plantuml.com/plantuml](https://www.plantuml.com/plantuml/uml) — только если допустимо по правилам курса.

## Правки

Текст в овалах и шагах должен совпадать с отчётом. Меняйте подписи в `.puml` и пересобирайте картинки — это и есть «диаграммы в формате кода».

## Прецеденты: почему кривые стрелки и что делать

**Тип диаграммы** (прецеденты) менять не нужно — меняется **рендерер**. Activity в PlantUML почти всегда нормальный; use case Graphviz кладёт криво.

1. **`use-case.puml` уже с `!pragma layout smetana`** — пересоберите PNG/SVG. В новом PlantUML можно попробовать `!pragma layout elk` вместо `smetana`.
2. **Экспорт в SVG**, правка 2–3 линий в Inkscape/Figma — в отчёт PNG; исходник остаётся `.puml`.
3. **Два рисунка** (оператор / пассажир) — меньше пересечений, тот же тип UML.
4. **Онлайн Kroki** или jar PlantUML 1.2024+ — иногда лучше плагина в IDE.

Прямоугольные «аккуратные» прецеденты как в Word без правок **из кода** почти нигде не получаются; для отчёта часто делают **текст в git (`puml`) + чуть подправленная картинка**.
