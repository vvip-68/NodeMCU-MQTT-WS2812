import {Injectable, OnDestroy} from '@angular/core';
import {interval, Observable, Observer, Subject, Subscription, takeWhile} from 'rxjs';
import {distinctUntilChanged, filter, map, share} from 'rxjs/operators';
import {WebSocketSubject, WebSocketSubjectConfig} from 'rxjs/webSocket';
import {environment} from "../../../environments/environment";

export const WS = {
  ON: {STATE: 'stt'},
  SEND: {COMMAND: 'cmd'}
};

export interface IWebsocketService {
  on<T>(event: string): Observable<T>;

  send(event: string, data: any): void;

  sendText(text: string): void;

  status$: Observable<boolean>;
}

export interface IWsMessage<T> {
  e: string;
  d: T;
}

@Injectable({
  providedIn: 'root'
})
export class WebsocketService implements IWebsocketService, OnDestroy {

  private config: WebSocketSubjectConfig<IWsMessage<any>>;

  private websocketSub: Subscription;
  private statusSub: Subscription;

  // Observable для реконнекта по interval
  private reconnection$: Observable<number> | null = null;
  private websocket$: WebSocketSubject<IWsMessage<any>> | null = null;

  // сообщает, когда происходит коннект и реконнект
  private connection$: Observer<boolean> | null = null

  // вспомогательный Observable для работы с подписками на сообщения
  private wsMessages$: Subject<IWsMessage<any>>;

  private url = environment.production ? `ws://${window.location.hostname}/ws` : "ws://192.168.0.120/ws";

  private pingInterval = 2500;
  private reconnectInterval = 5000;
  private sentPing = false;
  private gotPong = false;

  // -------------------------------------

  public firstConnect = true;
  public isConnected = false;

  // статус соединения
  public status$: Observable<boolean>;

  // -------------------------------------

  constructor() {

    this.config = {
      url: this.url,
      closeObserver: {
        next: (event: CloseEvent) => {
          console.log('WebSocket disconnected!');
          this.websocket$ = null;
          this.connection$?.next(false);
          this.reconnect();
        }
      },
      openObserver: {
        next: (event: Event) => {
          console.log('WebSocket connected!');
          this.firstConnect = false;
          this.connection$?.next(true);
        }
      }
    };

    // статус соединения
    this.status$ = new Observable<boolean>((observer) => {
      this.connection$ = observer;
    }).pipe(share(), distinctUntilChanged());

    // запускаем реконнект при отсутствии соединения
    this.statusSub = this.status$
      .subscribe((isConnected) => {
        this.isConnected = isConnected;
        if (isConnected) {
          document.body.classList.remove('noscroll');
        } else {
          document.body.classList.add('noscroll');
        }
      });

    // говорим, что что-то пошло не так + проверка полученных в сокет данных
    this.wsMessages$ = new Subject<IWsMessage<any>>();
    this.websocketSub = this.wsMessages$.subscribe(
      (data: IWsMessage<any>) => {
        // console.log(data);
      },
      (error: ErrorEvent) => {
        console.error('WebSocket error!', error);
        this.connection$?.next(false);
      }
    );

    // Интервал проверки соединения - отвечает ли сервер (ping-pong)
    interval(this.pingInterval).subscribe((i) => {
      if (this.isConnected) {
        if (this.sentPing && !this.gotPong) {
          this.connection$?.next(false);
        }
        this.ping();
      }
    });

    this.connect();
  }

  private connect(): void {
    this.websocket$ = new WebSocketSubject(this.config); // создаем
    // если есть сообщения, шлем их в дальше,
    // если нет, ожидаем
    // реконнектимся, если получили ошибку
    this.websocket$
      .subscribe(
        (message: IWsMessage<any>) => this.wsMessages$.next(message),
        (error: Event) => {
          if (!this.websocket$) {
            // run reconnect if errors
            this.connection$?.next(false);
            this.reconnect();
          }
        });
  }

  private reconnect(): void {
    // Создаем interval со значением из reconnectInterval
    this.reconnection$ = interval(this.reconnectInterval)
      .pipe(takeWhile((v, index) => !this.websocket$));

    // Пытаемся подключиться пока не подключимся, либо не упремся в ограничение попыток подключения
    this.reconnection$.subscribe(
      () => this.connect(),
      null,
      () => {
        // Subject complete if reconnect attemts ending
        this.reconnection$ = null;

        if (!this.websocket$) {
          this.wsMessages$.complete();
          this.connection$?.complete();
        }
      });
  }

  /*
  * send ping message to client
  * */
  private ping() {
    if (this.websocket$) {
      this.websocket$?.next({e: '?', d: ''});
      this.sentPing = true;
      this.gotPong = false;
    } else {
      this.reconnect();
    }
  }

  /*
  * answer on ping received
  * */
  public pong() {
    this.sentPing = false;
    this.gotPong = false;
    this.connection$?.next(true);
  }

  /*
  * on message event
  * */
  public on = <T>(event: string): Observable<T> => {
    return this.wsMessages$.pipe(
      filter((message: IWsMessage<any>) => message.e === event),
      map((message: IWsMessage<any>) => message.d)
    );
  };

  /*
  * on message to server
  * */
  public send(event: string, data: any = {}): void {
    if (this.isConnected) {
      if (event === WS.SEND.COMMAND) {
        this.websocket$?.next({e: event, d: data});
      }
    }
  }

  /*
  * on send text command message to server
  * */
  public sendText(text: string): void {
    this.send(WS.SEND.COMMAND, text);
  }

  ngOnDestroy() {
  }
}
