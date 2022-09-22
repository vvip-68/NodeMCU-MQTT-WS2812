import {RGBA} from "ngx-color";

export interface IEffectModel {
  id: number;       // id эффекта
  name: string;     // имя эффекта
  fav: boolean;     // флаг: выбранный для автовоспроизведения в цикле эффектов
  duration: number; //  T - время "проигрывания" режима 15..255 сек
  speed: number;    //  D - задержка между циклами (т.е. фактически задает скорость "проигрывания" режима 0..255 мс;  0 - нет задержки -> макс скорость
  segments: number; //  S - число сегментов разбиения ленты для режима - 1..6 (для режимов, которые поддерживают сегменты - /5,6,7,8,11,14,16,18,19,22,23,34,35,36,37,38/)
  step: number;     //  P - значение шага изменения параметра - 1..12 (для режимов, которые поддерживают - /3,17,39,40,41,42/) - скорость "затухания" или смены цвета
  active: boolean;  //  Флаг активного режима
}

export class EffectModel implements IEffectModel {

  public id: number = 0;
  public name: string = "";
  public fav: boolean = false;
  public duration: number = 30;
  public speed: number = 100;
  public segments: number = 1;
  public step: number = 4;
  public active = false;

  constructor(id?: number, name?: string, fav?: boolean) {
    this.id = id ?? 0;
    this.name = name ?? '';
    this.fav = fav ?? false;
  }
}

// -------------------------------------------------

export type OnOffFunction = (a: any) => string;
export type CommandFunction = (a: any) => string;

export enum ActionType {
  NONE = 'none',
  POWER = 'power',
  BRIGHT = 'bright',
  COLOR = 'color',
  EFFECT = 'effect'
}

export interface IActionModel {
  id: number;                       // ID режима
  readonly key: string;             // Ключ команды, по которой менеджер Эффектов устанавливает актуальные данные в поле value
  mode: ActionType;                 // Тип действия
  submode: string;                  // Вариант действия
  value: any;                       // Значение параметра
  onoff: boolean | OnOffFunction;   // Функция проверки состояния: возвращает строку класса
  get: string | CommandFunction;    // Команда запроса параметров
  set: string | CommandFunction;    // Команда установки параметра
  active: boolean;                  // Флаг активного режима
}


export class ActionModel implements IActionModel {

  public id = 0;
  public key = '';
  public mode = ActionType.NONE;
  public submode = '';
  public get: string | CommandFunction = (val: any) => '';
  public set: string | CommandFunction = (val: any) => '';
  public value: any = null;
  public onoff: boolean | OnOffFunction = (val: any) => '';
  public active = false;

  constructor(data?: object) {
    if (data) {
      Object.assign(this, data);
    }
    switch (this.mode) {
      case ActionType.POWER:
        this.key = 'PWR';
        break;
      case ActionType.BRIGHT:
        this.key = 'BR';
        break;
      case ActionType.EFFECT:
        this.key = 'PM';
        break;
      case ActionType.COLOR:
        this.key = 'RGB';
        break;
    }
  }
}

// -------------------------------------------------

export interface IStateModel {
  version: string;        // Версия прошивки
  error: string;          // Сообщение о последней ошибке
  info: string;           // Последнее информационное сообщение
  brightness: number;     // Текущая яркость
  power: boolean;         // Состояние питания: вкл/выкл
  isRandom: boolean;      // Состояние "выбор случайного эффекта"
  mode: number;           // Текущий режим
  color: RGBA;            // Текущий установленный пользовательский цвет
}

export class StateModel implements IStateModel {
  public version = '';
  public error = '';
  public info = '';
  public brightness = -1;
  public power = false;
  public isRandom = false;
  public mode = 0;
  public color: RGBA = {r: 0, g: 0, b: 0, a: 1};

  constructor(data?: object) {
    if (data) {
      Object.assign(this, data);
    }
  }
}
