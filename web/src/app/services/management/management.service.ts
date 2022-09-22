import {Injectable, InjectionToken, OnDestroy} from '@angular/core';
import {BehaviorSubject, Observable, Subject, Subscription, take, takeUntil, timer} from "rxjs";
import {WebsocketService, WS} from "../websocket/websocket.service";
import {EffectModel, IActionModel, IEffectModel, StateModel} from "../../models/effect.model";

enum MessageType {
  INFO,
  ERROR
}

@Injectable({
  providedIn: 'root'
})
export class ManagementService implements OnDestroy {
  public static management = new InjectionToken<ManagementService>('Management Token');

  public actions: Array<IActionModel> = [];
  public effects: Array<IEffectModel> = [];

  public editModel$ = new BehaviorSubject<EffectModel | null>(null);
  public state$ = new BehaviorSubject<StateModel>(new StateModel());
  public effects$ = new BehaviorSubject<Array<EffectModel>>([]);
  public data$: Observable<string>;

  private timer: Subscription | null = null;

  private destroy$ = new Subject();
  private state = new StateModel();
  private editModel: EffectModel | null = null;

  constructor(private wsService: WebsocketService) {
    this.data$ = this.wsService.on<string>(WS.ON.STATE);
    this.data$
      .pipe(takeUntil(this.destroy$))
      .subscribe((data: string) => {
        this.processData(data);
      });
  }

  private timerMessage(type: MessageType, message: string) {
    if (this.timer) this.timer.unsubscribe();
    this.state.info = '';
    this.state.error = '';

    if (type == MessageType.INFO)
      this.state.info = message;
    else
      this.state.error = message;

    this.timer = timer(5000).pipe(take(1)).subscribe(data => {
      this.state.info = '';
      this.state.error = '';
      this.timer = null;
    });
  }

  private processData(data: string) {
    if (!data) return;

    if (data == '!') {
      this.wsService.pong();
      return;
    }

    const idx = data.indexOf(':');
    if (idx <= 0) return;

    const cmd = data.slice(0, idx).trim().toUpperCase();
    const args = data.substring(idx + 1).trim();
    if (cmd.length === 0 || args.length === 0) return;

    console.log("cmd='%s'; args='%s'", cmd, args)

    switch (cmd) {
      // Версия прошивки
      case 'VER': {
        this.state.version = args;
        this.state$.next(this.state);
        break;
      }

      // Текст сообщений об ошибке
      case 'ERR': {
        this.timerMessage(MessageType.ERROR, args);
        this.state$.next(this.state);
        break;
      }

      // Текст информационного сообщения
      case 'NFO': {
        this.timerMessage(MessageType.INFO, args);
        this.state$.next(this.state);
        break;
      }

      // Текущее состояние питания - PWR:ON - включено, PWR:OFF - выключено
      case 'PWR': {
        this.state.power = args.toUpperCase() === 'ON';
        this.state$.next(this.state);
        break;
      }

      // Текущая яркость: BR:N где N - 0..255
      case 'BR': {
        this.state.brightness = parseInt(args);
        this.state$.next(this.state);
        if (this.state.error.length === 0) {
          this.timerMessage(MessageType.INFO, `Установлена яркость ${Math.round(this.state.brightness * 100 / 255)}%`);
        }
        break;
      }

      // Состояние режима "Случайный порядок" эффектов - RND:ON - включено, RND:OFF - выключено
      case 'RND': {
        this.state.isRandom = args.toUpperCase() === 'ON';
        this.state$.next(this.state);
        break;
      }

      // Уведомление об эффекте и его параметрах
      // PM:N:T:D:S:P:U:A - параметры режима N:
      //  N - [0] номер режима - 2..MAX_EFFECT
      //  T - [1] время "проигрывания" режима 15..255 сек
      //  D - [2] задержка между циклами (т.е. фактически задает скорость "проигрывания" режима
      //  S - [3] число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
      //  P - [4] значение шага изменения параметра (для режимов, которые поддерживают - /3,17,39,40,41,42/)
      //  U - [5] 0 - не используется в автоматической смене режимов, 1 - режим используется
      //  A - [6] 0 - параметры режима для редактирования, 1 - параметры текущего активного режима
      case 'PM': {
        const parts = args.split(':');
        if (parts[6] == '1') {
          // Сменить активность карточки
          this.setActive(this.state.mode, false);
          // Получены параметры текущего активного режима (произошло включение режима)
          this.state.mode = parseInt(parts[0]);
          this.setActive(this.state.mode, true);
          if (this.state.mode >= 99 && this.state.mode <= 107) {
            // Параметры специального режима - выбранного цвета
            //  99 - включить черный цвет (устройство остается включенными)
            // 100 - включить белый цвет
            // 101 - включить красный цвет
            // 102 - включить зеленый цвет
            // 103 - включить синий цвет
            // 104 - включить желтый цвет
            // 105 - включить голубой цвет
            // 106 - включить сиреневый цвет
            // 107 - включить цвет, ранее установленный пользователем командой "RGB"
            if (this.state.error.length === 0) {
              this.timerMessage(MessageType.INFO, 'Выбран цвет ленты из палитры');
            }
          } else {
            // Параметры режима (эффекта)  2..MAX_EFFECT
            const idx = this.effects.findIndex((effect) => effect.id === this.state.mode);
            if (idx >= 0) {
              this.effects[idx].duration = parseInt(parts[1]);
              this.effects[idx].speed = parts[2] === 'X' ? 0 : parseInt(parts[2]);
              this.effects[idx].segments = parts[3] === 'X' ? 0 : parseInt(parts[3]);
              this.effects[idx].step = parts[4] === 'X' ? 0 : parseInt(parts[4]);
              this.effects[idx].fav = parts[5] === '1';

              const effect_name = this.effects[idx].name ?? '';
              if (this.state.error.length === 0 && effect_name.length > 0) {
                this.timerMessage(MessageType.INFO, `Включен эффект '${effect_name}'`);
              }
            }
            this.state$.next(this.state);
          }
        } else {
          // Получены данные редактируемой модели
          const id = parseInt(parts[0]);
          const idx = this.effects.findIndex((effect) => effect.id === id);
          if (idx >= 0) {
            const name = this.effects[idx].name;
            const fav = parts[5] === '1';
            const effect = new EffectModel(id, name, fav)
            effect.duration = parseInt(parts[1]);
            effect.speed = parts[2] === 'X' ? 0 : parseInt(parts[2]);
            effect.segments = parts[3] === 'X' ? 0 : parseInt(parts[3]);
            effect.step = parts[4] === 'X' ? 0 : parseInt(parts[4]);
            this.editModel$.next(effect);
          }
        }
        break;
      }

      // Текущий установленный цвет пользователя RGB:R:G:B  R,G,B - 0..255
      case 'RGB': {
        const parts = args.split(':');
        this.state.color.r = parseInt(parts[0]);
        this.state.color.g = parseInt(parts[1]);
        this.state.color.b = parseInt(parts[2]);
        this.state.color.a = 1;
        this.state$.next(this.state);
        break;
      }

      // Список эффектов, отмеченных как "любимые" - для использования в цикле автоматического включения
      case 'FAV': {
        const parts = args.split(',');
        for (const part of parts) {
          const id = parseInt(part);
          const idx = this.effects.findIndex((effect) => effect.id === id);
          if (idx >= 0) {
            this.effects[idx].fav = true;
          }
        }
        this.effects$.next(this.effects);
        break;
      }

      // Список эффектов в формате N[имя]:N[имя]:N[имя], где N - ID эффекта 2..MAX_EFFECT, 'имя' - имя эффекта для отображения
      case 'LST': {
        this.effects = [];
        const parts = args.split(':');
        for (const part of parts) {
          const i = part.indexOf('[');
          const id = parseInt(part.slice(0, i));
          const name = part.substring(i + 1, part.length - 1);
          this.effects.push(new EffectModel(id, name));
        }
        this.effects$.next(this.effects);
        break;
      }
    }
  }

  private setActive(active_id: number, active: boolean) {
    if (active_id >= 99 && active_id <= 107) {
      const action = this.actions.find((act) => act.id == active_id);
      if (action) {
        action.active = active;
      }
    } else {
      const effect = this.effects.find((act) => act.id == active_id);
      if (effect) {
        effect.active = active;
      }
    }

  }

  public prepareEdit(id: number) {
    this.editModel = null;
    this.editModel$.next(this.editModel);
    this.wsService.sendText(`EDT:${id}`);
  }

  ngOnDestroy() {
    this.destroy$.next(true);
    this.destroy$.complete();
  }
}
